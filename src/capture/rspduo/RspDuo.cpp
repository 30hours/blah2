#include "RspDuo.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

// class static constants
const double RspDuo::MAX_FREQUENCY_NR = 2000000000;
const uint8_t RspDuo::DEF_DECIMATION_NR = 1;
const int RspDuo::DEF_WAIT_TIME_NR = 0;              // default wait time before recording
const int RspDuo::DEF_CHUNK_TIME_NR = 0;             // default chunk time of recording
const int RspDuo::MAX_RUN_TIME_NR = 86400;           // max run time of recording
const int RspDuo::DEF_AGC_BANDWIDTH_NR = 50;         // default agc bandwidth
const int RspDuo::MIN_AGC_SET_POINT_NR = -72;        // min agc set point
const int RspDuo::DEF_AGC_SET_POINT_NR = -60;        // default agc set point
const int RspDuo::MIN_GAIN_REDUCTION_NR = 20;        // min gain reduction
const int RspDuo::DEF_GAIN_REDUCTION_NR = 40;        // default gain reduction
const int RspDuo::MAX_GAIN_REDUCTION_NR = 60;        // max gain reduction
const int RspDuo::DEF_LNA_STATE_NR = 4;              // default lna state
const int RspDuo::MAX_LNA_STATE_NR = 9;              // max lna state
const int RspDuo::DEF_SAMPLE_FREQUENCY_NR = 6000000; // default sample frequency
const int RspDuo::DEF_SAMPLE_RATE_NR = 2000000;      // default sample rate

// global variables (SDRPlay)
sdrplay_api_DeviceT *chosenDevice = NULL;
sdrplay_api_DeviceT devs[1023];
sdrplay_api_DeviceParamsT *deviceParams = NULL;
sdrplay_api_ErrT err;
sdrplay_api_CallbackFnsT cbFns;
sdrplay_api_RxChannelParamsT *chParams;

// global variables
//FILE *out_file_fp = NULL;
FILE *file_replay = NULL;
short *buffer_16_ar = NULL;
struct timeval current_tm = {0, 0};
struct timeval start_tm = {0, 0};
struct timeval chunk_tm = {0, 0};
struct timeval finish_tm = {0, 0};
uint32_t frames_nr = 0;
std::string file;
short max_a_nr = 0;
short max_b_nr = 0;
bool run_fg = true;
bool stats_fg = true;
bool write_fg = true;
bool *capture_fg;
std::ofstream* saveIqFileLocal;
int wait_time_nr = 2;
IqData *buffer1;
IqData *buffer2;

// constructor
RspDuo::RspDuo(std::string _type, uint32_t _fc, uint32_t _fs, 
  std::string _path, bool *_saveIq)
    : Source(_type, _fc, _fs, _path, _saveIq)
{
  nDecimation = DEF_DECIMATION_NR;
  usb_bulk_fg = false;
  small_verbose_fg = false;
  more_verbose_fg = false;
  agc_bandwidth_nr = DEF_AGC_BANDWIDTH_NR;
  agc_set_point_nr = DEF_AGC_SET_POINT_NR;
  gain_reduction_nr = DEF_GAIN_REDUCTION_NR;
  lna_state_nr = DEF_LNA_STATE_NR;
  rf_notch_fg = false;
  dab_notch_fg = false;
  chunk_time_nr = DEF_CHUNK_TIME_NR;

  //out_file_fp = saveIqFile;
  capture_fg = saveIq;
  saveIqFileLocal = &saveIqFile;
}

void RspDuo::start()
{
  open_api();
  get_device();
  set_device_parameters();
  validate();
}

void RspDuo::stop()
{
  uninitialise_device();
  finish();
}

void RspDuo::process(IqData *_buffer1, IqData *_buffer2)
{
  buffer1 = _buffer1;
  buffer2 = _buffer2;

  initialise_device();

  // control loop
  while (run_fg)
  {
    if (write_fg)
    {
      fprintf(stderr, "Info - control_loop - frames_nr: %d max_a_nr: %d max_b_nr: %d\n", frames_nr, max_a_nr, max_b_nr);
      max_a_nr = 0;
      max_b_nr = 0;
    }

    sleep(1);
  }
}

void RspDuo::replay(IqData *_buffer1, IqData *_buffer2, std::string _file, bool _loop)
{
  buffer1 = _buffer1;
  buffer2 = _buffer2;

  short i1, q1, i2, q2;
  int rv;
  file_replay = fopen(_file.c_str(), "rb");

  while (true)
  {
    rv = fread(&i1, 1, sizeof(short), file_replay);
    rv = fread(&q1, 1, sizeof(short), file_replay);
    rv = fread(&i2, 1, sizeof(short), file_replay);
    rv = fread(&q2, 1, sizeof(short), file_replay);

    buffer1->lock();
    buffer2->lock();

    if (buffer1->get_length() < buffer1->get_n())
    {
      buffer1->push_back({(double)i1, (double)q1});
      buffer2->push_back({(double)i2, (double)q2});
    }

    buffer1->unlock();
    buffer2->unlock();

  }

}

void RspDuo::validate()
{
  // validate decimation
  if (this->nDecimation != 1 && this->nDecimation != 2 && this->nDecimation != 4 &&
      this->nDecimation != 8 && this->nDecimation != 16 && this->nDecimation != 32)
  {
    fprintf(stderr, "Error - read_command_line - decimation must be in 1, 2, 4, 8, 16, 32\n");
    exit(1);
  }

  // validate fc
  if (this->fc < 1 || this->fc > MAX_FREQUENCY_NR)
  {
    fprintf(stderr, "Error - read_command_line - frequency must be between 1 and %.1f\n", MAX_FREQUENCY_NR);
    exit(1);
  }

  // validate agc
  if (agc_bandwidth_nr != 0 && agc_bandwidth_nr != 5 && agc_bandwidth_nr != 50 && agc_bandwidth_nr != 100)
  {
    fprintf(stderr, "Error - read_command_line - agc bandwidth must be in 0, 5, 50, 100\n");
    exit(1);
  }
  if (agc_set_point_nr > 0 || agc_set_point_nr < MIN_AGC_SET_POINT_NR)
  {
    fprintf(stderr, "Error - read_command_line - agc set point must be between %d and 0\n", MIN_AGC_SET_POINT_NR);
    exit(1);
  }

  // validate LNA
  if (gain_reduction_nr < MIN_GAIN_REDUCTION_NR || gain_reduction_nr > MAX_GAIN_REDUCTION_NR)
  {
    fprintf(stderr, "Error - read_command_line - gain reduction must be between %d and %d\n", MIN_GAIN_REDUCTION_NR, MAX_GAIN_REDUCTION_NR);
    exit(1);
  }
  if (lna_state_nr < 1 || lna_state_nr > MAX_LNA_STATE_NR)
  {
    fprintf(stderr, "Error - read_command_line - lna state must be between 1 and %d\n", MAX_LNA_STATE_NR);
    exit(1);
  }

  // validate notch filters

  // validate wait/chunk
  if (wait_time_nr < 0 || wait_time_nr > MAX_RUN_TIME_NR)
  {
    fprintf(stderr, "Error - read_command_line - wait time must be between 0 and %d\n", MAX_RUN_TIME_NR);
    exit(1);
  }
  if (chunk_time_nr < 0 || chunk_time_nr > MAX_RUN_TIME_NR)
  {
    fprintf(stderr, "Error - read_command_line - chunk time must be between 0 and %d\n", MAX_RUN_TIME_NR);
    exit(1);
  }

  // validate the command line options - other
  if (small_verbose_fg && more_verbose_fg)
  {
    fprintf(stderr, "Error - read_command_line - use only one of -v/-V\n");
    exit(1);
  }

  // print them out
  fprintf(stderr, "\n");
  fprintf(stderr, "fc (Hz)                       : %d\n", this->fc);
  fprintf(stderr, "file                          : %s\n", file.c_str());
  fprintf(stderr, "wait_time_nr (s)              : %d\n", wait_time_nr);
  fprintf(stderr, "chunk_time_nr (s)             : %d\n", chunk_time_nr);
  fprintf(stderr, "agc_bandwidth_nr (Hz)         : %d\n", agc_bandwidth_nr);
  fprintf(stderr, "agc_set_point_nr (dBfs)       : %d\n", agc_set_point_nr);
  fprintf(stderr, "gain_reduction_nr (dB)        : %d\n", gain_reduction_nr);
  fprintf(stderr, "lna_state_nr                  : %d\n", lna_state_nr);
  fprintf(stderr, "N_DECIMATION                  : %d\n", this->nDecimation);
  fprintf(stderr, "rf_notch_fg                   : %s\n", rf_notch_fg ? "true" : "false");
  fprintf(stderr, "dab_notch_fg                  : %s\n", dab_notch_fg ? "true" : "false");
  fprintf(stderr, "usb_bulk_fg                   : %s\n", usb_bulk_fg ? "true" : "false");
  fprintf(stderr, "stats_fg                      : %s\n", stats_fg ? "true" : "false");
  fprintf(stderr, "small_verbose_fg              : %s\n", small_verbose_fg ? "true" : "false");
  fprintf(stderr, "more_verbose_fg               : %s\n", more_verbose_fg ? "true" : "false");
  fprintf(stderr, "\n");

  return;
}

void RspDuo::open_api()
{
  float ver = 0.0;

  // open the sdrplay api
  if ((err = sdrplay_api_Open()) != sdrplay_api_Success)
  {
    fprintf(stderr, "Error - sdrplay_api_Open failed %s\n", sdrplay_api_GetErrorString(err));
    exit(1);
  }
  // enable debug logging output
  if ((err = sdrplay_api_DebugEnable(NULL, sdrplay_api_DbgLvl_Verbose)) != sdrplay_api_Success)
  {
    fprintf(stderr, "Error - sdrplay_api_DebugEnable failed %s\n", sdrplay_api_GetErrorString(err));
    sdrplay_api_Close();
    exit(1);
  }

  // check api versions match
  if ((err = sdrplay_api_ApiVersion(&ver)) != sdrplay_api_Success)
  {
    fprintf(stderr, "Error - sdrplay_api_ApiVersion failed %s\n", sdrplay_api_GetErrorString(err));
    sdrplay_api_Close();
    exit(1);
  }

  if (ver != SDRPLAY_API_VERSION)
  {
    fprintf(stderr, "Error - open_api - API versions do not match (local=%.2f dll=%.2f)\n", SDRPLAY_API_VERSION, ver);
    sdrplay_api_Close();
    exit(1);
  }

  return;
}

void RspDuo::get_device()
{
  int i;
  unsigned int ndev;
  unsigned int chosenIdx = 0;

  // lock api while device selection is performed
  if ((err = sdrplay_api_LockDeviceApi()) != sdrplay_api_Success)
  {
    fprintf(stderr, "Error - sdrplay_api_LockDeviceApi failed %s\n", sdrplay_api_GetErrorString(err));
    sdrplay_api_Close();
    exit(1);
  }

  // fetch list of available devices
  if ((err = sdrplay_api_GetDevices(devs, &ndev, sizeof(devs) / sizeof(sdrplay_api_DeviceT))) != sdrplay_api_Success)
  {
    fprintf(stderr, "Error - sdrplay_api_GetDevices failed %s\n", sdrplay_api_GetErrorString(err));
    sdrplay_api_UnlockDeviceApi();
    sdrplay_api_Close();
    exit(1);
  }

  fprintf(stderr, "Info - get_device - MaxDevs=%ld NumDevs=%d\n", sizeof(devs) / sizeof(sdrplay_api_DeviceT), ndev);

  if (ndev == 0)
  {
    fprintf(stderr, "Error - get_device - no devices found\n");
    sdrplay_api_UnlockDeviceApi();
    sdrplay_api_Close();
    exit(1);
  }

  // pick first RSPduo
  for (i = 0; i < ndev; i++)
  {
    if (devs[i].hwVer == SDRPLAY_RSPduo_ID)
    {
      chosenIdx = i;
      fprintf(stderr, "Info - get_device - Dev%d: SerNo=%s hwVer=%d tuner=0x%.2x rspDuoMode=0x%.2x\n",
              i, devs[i].SerNo, devs[i].hwVer, devs[i].tuner, devs[i].rspDuoMode);
      break;
    }
  }

  if (i == ndev)
  {
    fprintf(stderr, "Error - get_device - could not find a suitable RSPduo device to open\n");
    sdrplay_api_UnlockDeviceApi();
    sdrplay_api_Close();
    exit(1);
  }

  chosenDevice = &devs[chosenIdx];

  fprintf(stderr, "Info - get_device - chosenDevice=%d\n", chosenIdx);

  // set operating mode
  chosenDevice->tuner = sdrplay_api_Tuner_Both;
  chosenDevice->rspDuoMode = sdrplay_api_RspDuoMode_Dual_Tuner;
  chosenDevice->rspDuoSampleFreq = DEF_SAMPLE_FREQUENCY_NR;

  fprintf(stderr, "Info - get_device - Dev%d: selected rspDuoMode=0x%.2x tuner=0x%.2x rspDuoSampleFreq=%.1f\n",
          chosenIdx, chosenDevice->rspDuoMode, chosenDevice->tuner, chosenDevice->rspDuoSampleFreq);

  // select chosen device
  if ((err = sdrplay_api_SelectDevice(chosenDevice)) != sdrplay_api_Success)
  {
    fprintf(stderr, "Error - sdrplay_api_SelectDevice failed %s\n", sdrplay_api_GetErrorString(err));
    sdrplay_api_UnlockDeviceApi();
    sdrplay_api_Close();
    exit(1);
  }

  // unlock api now that device is selected
  if ((err = sdrplay_api_UnlockDeviceApi()) != sdrplay_api_Success)
  {
    fprintf(stderr, "Error - sdrplay_api_UnlockDeviceApi failed %s\n", sdrplay_api_GetErrorString(err));
    sdrplay_api_Close();
    exit(1);
  }

  return;
}

void RspDuo::set_device_parameters()
{
  // retrieve device parameters so they can be changed if wanted
  if ((err = sdrplay_api_GetDeviceParams(chosenDevice->dev, &deviceParams)) != sdrplay_api_Success)
  {
    std::cerr << "Error - sdrplay_api_GetDeviceParams failed " + std::string(sdrplay_api_GetErrorString(err)) << std::endl;
    sdrplay_api_Close();
    exit(1);
  }

  // check for NULL pointer before changing settings
  if (deviceParams == NULL)
  {
    std::cerr << "Error - sdrplay_api_GetDeviceParams returned NULL deviceParams pointer" << std::endl;
    sdrplay_api_Close();
    exit(1);
  }

  // set USB mode
  if (usb_bulk_fg)
  {
    deviceParams->devParams->mode = sdrplay_api_BULK;
  }
  else
  {
    deviceParams->devParams->mode = sdrplay_api_ISOCH;
  }

  // configure channels - these affect both channels identically
  chParams = deviceParams->rxChannelA;

  // check for NULL pointer before changing settings
  if (chParams == NULL)
  {
    std::cerr << "Error - sdrplay_api_GetDeviceParams returned NULL chParams pointer" << std::endl;
    sdrplay_api_Close();
    exit(1);
  }

  // set center frequency
  chParams->tunerParams.rfFreq.rfHz = this->fc;

  // set AGC
  chParams->ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;
  if (agc_bandwidth_nr == 5)
  {
    chParams->ctrlParams.agc.enable = sdrplay_api_AGC_5HZ;
  }
  else if (agc_bandwidth_nr == 50)
  {
    chParams->ctrlParams.agc.enable = sdrplay_api_AGC_50HZ;
  }
  else if (agc_bandwidth_nr == 100)
  {
    chParams->ctrlParams.agc.enable = sdrplay_api_AGC_100HZ;
  }
  if (chParams->ctrlParams.agc.enable != sdrplay_api_AGC_DISABLE)
  {
    chParams->ctrlParams.agc.setPoint_dBfs = (0 < agc_set_point_nr) ? 0 : agc_set_point_nr;
  }

  // set gain reduction and lna sate
  chParams->tunerParams.gain.gRdB = gain_reduction_nr;
  chParams->tunerParams.gain.LNAstate = lna_state_nr;

  // set decimation and IF frequency and analog bandwidth
  chParams->ctrlParams.decimation.enable = 1;
  chParams->ctrlParams.decimation.decimationFactor = this->nDecimation;
  chParams->tunerParams.ifType = sdrplay_api_IF_1_620;
  chParams->tunerParams.bwType = sdrplay_api_BW_1_536;

  if (this->nDecimation == 4)
  {
    // 2 MSa/s / 4 = 500 kHz
    chParams->tunerParams.bwType = sdrplay_api_BW_0_600;
  }
  else if (this->nDecimation == 8)
  {
    // 2 MSa/s / 8 = 250 kHz
    chParams->tunerParams.bwType = sdrplay_api_BW_0_300;
  }
  else if (this->nDecimation == 16 || this->nDecimation == 32)
  {
    // 2 MSa/s / 16 = 125 kHz
    // 2 MSa/s / 32 = 62.5 kHz
    chParams->tunerParams.bwType = sdrplay_api_BW_0_200;
  }

  // configure notch filters
  chParams->rspDuoTunerParams.rfNotchEnable = rf_notch_fg;
  chParams->rspDuoTunerParams.rfDabNotchEnable = dab_notch_fg;

  // assign callback functions to be passed to sdrplay_api_Init()
  cbFns.StreamACbFn = _stream_a_callback;
  cbFns.StreamBCbFn = _stream_b_callback;
  cbFns.EventCbFn = _event_callback;

  return;
}

void RspDuo::stream_a_callback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, unsigned int numSamples, unsigned int reset, void *cbContext)
{

  int i = 0;
  int j = 0;

  // process stream callback data
  buffer_16_ar = (short int *)malloc(numSamples * 4 * sizeof(short));

  if (buffer_16_ar == NULL)
  {
    std::cerr << "Error - stream_a_callback - malloc failed" << std::endl;
    run_fg = false;
    return;
  }

  // IIQQxxxx
  for (i = 0; i < numSamples; i++)
  {
    // add tuner A data
    buffer_16_ar[j++] = xi[i];
    buffer_16_ar[j++] = xq[i];
    // skip tuner B data
    j++;
    j++;
  }

  // find max for stats
  if (stats_fg && write_fg)
  {
    for (i = 0; i < numSamples; i++)
    {
      if (xi[i] > max_a_nr)
      {
        max_a_nr = xi[i];
      }
    }
  }

  return;
}

void RspDuo::stream_b_callback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, unsigned int numSamples, unsigned int reset, void *cbContext)
{
  int i = 0;
  int j = 0;

  // xxxxIIQQ
  for (i = 0; i < numSamples; i++)
  {
    // skip tuner A data
    j++;
    j++;
    // add tuner B data
    buffer_16_ar[j++] = xi[i];
    buffer_16_ar[j++] = xq[i];
  }

  // write data to IqData
  buffer1->lock();
  buffer2->lock();
  for (int i = 0; i < numSamples*4; i+=4)
  {
    buffer1->push_back({(double)buffer_16_ar[i], (double)buffer_16_ar[i+1]});
    buffer2->push_back({(double)buffer_16_ar[i+2], (double)buffer_16_ar[i+3]});
  }
  buffer1->unlock();
  buffer2->unlock();

  // decide if to write data
  gettimeofday(&current_tm, NULL);

  // write if over start time
  if (start_tm.tv_sec + wait_time_nr <= current_tm.tv_sec)
  {
    write_fg = true;
  }

  // write data to file
  if (*capture_fg && write_fg)
  {
    saveIqFileLocal->write(reinterpret_cast<char*>(buffer_16_ar), 
      sizeof(short) * numSamples * 4);
    
    if (!(*saveIqFileLocal))
    {
      std::cerr << "Error - stream_b_callback - not enough samples received" << std::endl;
      free(buffer_16_ar);
      run_fg = false;
      return;
    }

    frames_nr = frames_nr + numSamples;
  }

  free(buffer_16_ar);

  // find max for stats
  if (stats_fg && write_fg)
  {
    for (i = 0; i < numSamples; i++)
    {
      if (xi[i] > max_b_nr)
      {
        max_b_nr = xi[i];
      }
    }
  }

  return;
}

void RspDuo::event_callback(sdrplay_api_EventT eventId, sdrplay_api_TunerSelectT tuner, sdrplay_api_EventParamsT *params, void *cbContext)
{
  switch (eventId)
  {
  case sdrplay_api_GainChange:
    fprintf(stderr, "Info - event_callback - GainChange tuner=%s gRdB=%d lnaGRdB=%d systemGain=%.2f\n",
            (tuner == sdrplay_api_Tuner_A) ? "sdrplay_api_Tuner_A" : "sdrplay_api_Tuner_B",
            params->gainParams.gRdB, params->gainParams.lnaGRdB, params->gainParams.currGain);
    break;

  case sdrplay_api_PowerOverloadChange:
    fprintf(stderr, "Info - event_callback - PowerOverloadChange tuner=%s powerOverloadChangeType=%s\n",
            (tuner == sdrplay_api_Tuner_A) ? "sdrplay_api_Tuner_A" : "sdrplay_api_Tuner_B",
            (params->powerOverloadParams.powerOverloadChangeType == sdrplay_api_Overload_Detected) ? "sdrplay_api_Overload_Detected" : "sdrplay_api_Overload_Corrected");

    // send update message to acknowledge power overload message received
    sdrplay_api_Update(chosenDevice->dev, tuner, sdrplay_api_Update_Ctrl_OverloadMsgAck, sdrplay_api_Update_Ext1_None);
    break;

  case sdrplay_api_DeviceRemoved:
    fprintf(stderr, "Info - event_callback - device removed\n");
    break;

  default:
    fprintf(stderr, "Info - event_callback - unknown event %d\n", eventId);
    break;
  }
}

void RspDuo::initialise_device()
{
  if ((err = sdrplay_api_Init(chosenDevice->dev, &cbFns, NULL)) != sdrplay_api_Success)
  {
    fprintf(stderr, "Error - sdrplay_api_Init failed %s\n", sdrplay_api_GetErrorString(err));
    sdrplay_api_Close();
    exit(1);
  }
}

void RspDuo::uninitialise_device()
{
  if ((err = sdrplay_api_Uninit(chosenDevice->dev)) != sdrplay_api_Success)
  {
    fprintf(stderr, "Error - sdrplay_api_Uninit failed %s\n", sdrplay_api_GetErrorString(err));
    sdrplay_api_Close();
    exit(1);
  }

  sdrplay_api_ReleaseDevice(chosenDevice);
  sdrplay_api_UnlockDeviceApi();
  sdrplay_api_Close();
}

void RspDuo::finish()
{
  char time_tx[BUFFER_SIZE_NR];

  // close files
  close_file();

  // get finish date and time
  gettimeofday(&finish_tm, NULL);

  strftime(time_tx, sizeof(time_tx), "%d %b %Y %H:%M:%S", localtime(&finish_tm.tv_sec));
  fprintf(stderr, "\n");
  fprintf(stderr, "Info - finish - finish_tm: %s.%03ld\n", time_tx, finish_tm.tv_usec / 1000);
  fprintf(stderr, "Info - finish - frames_nr: %d\n", frames_nr);

  return;
}

