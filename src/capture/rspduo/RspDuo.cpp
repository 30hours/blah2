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
#include <unordered_map>
#include <iostream>

// class static constants
const double RspDuo::MAX_FREQUENCY_NR = 2000000000;
const int RspDuo::MIN_AGC_SET_POINT_NR = -72;        // min agc set point
const int RspDuo::MIN_GAIN_REDUCTION_NR = 20;        // min gain reduction
const int RspDuo::MAX_GAIN_REDUCTION_NR = 59;        // max gain reduction
const int RspDuo::MAX_LNA_STATE_NR = 9;              // max lna state
const int RspDuo::DEF_SAMPLE_RATE_NR = 2000000;      // default sample rate

// global variables (SDRPlay)
sdrplay_api_DeviceT *chosenDevice = NULL;
sdrplay_api_DeviceT devs[1023];
sdrplay_api_DeviceParamsT *deviceParams = NULL;
sdrplay_api_ErrT err;
sdrplay_api_CallbackFnsT cbFns;
sdrplay_api_RxChannelParamsT *chParams;

// global variables
FILE *file_replay = NULL;
short *buffer_16_ar = NULL;
std::string file;
short max_a_nr = 0;
short max_b_nr = 0;
bool run_fg = true;
bool stats_fg = true;
bool *capture_fg;
std::ofstream* saveIqFileLocal;
IqData *buffer1;
IqData *buffer2;

// constructor
RspDuo::RspDuo(std::string _type, uint32_t _fc, 
  uint32_t _fs, std::string _path, bool *_saveIq,
  int _agcSetPoint, int _bandwidthNumber, 
  int _gainReduction, int _lnaState,
  bool _dabNotch, bool _rfNotch)
  : Source(_type, _fc, _fs, _path, _saveIq)
{
  std::unordered_map<int, int> decimationMap = {
    {2000000, 1},
    {1000000, 2},
    {500000, 4},
    {250000, 8},
    {125000, 16},
    {62500, 32}
  };
  std::unordered_map<int, sdrplay_api_Bw_MHzT> ifBandwidthMap = {
    {2000000, sdrplay_api_BW_1_536},
    {1000000, sdrplay_api_BW_0_600},
    {500000, sdrplay_api_BW_0_600},
    {250000, sdrplay_api_BW_0_300},
    {125000, sdrplay_api_BW_0_200},
    {62500, sdrplay_api_BW_0_200}
  };
  std::unordered_map<int, sdrplay_api_If_kHzT> ifModeMap = {
    {2000000, sdrplay_api_IF_1_620},
    {1000000, sdrplay_api_IF_1_620},
    {500000, sdrplay_api_IF_1_620},
    {250000, sdrplay_api_IF_1_620},
    {125000, sdrplay_api_IF_1_620},
    {62500, sdrplay_api_IF_1_620}
  };
  nDecimation = decimationMap[fs];
  bwType = ifBandwidthMap[fs];
  ifType = ifModeMap[fs];
  usb_bulk_fg = false;
  capture_fg = saveIq;
  saveIqFileLocal = &saveIqFile;
  agc_bandwidth_nr = _bandwidthNumber;
  agc_set_point_nr = _agcSetPoint;
  gain_reduction_nr = _gainReduction;
  lna_state_nr = _lnaState;
  rf_notch_fg = _rfNotch;
  dab_notch_fg = _dabNotch;
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
}

void RspDuo::process(IqData *_buffer1, IqData *_buffer2)
{
  buffer1 = _buffer1;
  buffer2 = _buffer2;

  initialise_device();

  // control loop
  while (run_fg)
  {
    if (stats_fg)
    {
      std::cerr << "[RspDuo]" << " max_a_nr: " << max_a_nr << 
        " max_b_nr: " << max_b_nr << std::endl;
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
    if (rv != sizeof(short)) break; 
    rv = fread(&q1, 1, sizeof(short), file_replay);
    if (rv != sizeof(short)) break; 
    rv = fread(&i2, 1, sizeof(short), file_replay);
    if (rv != sizeof(short)) break; 
    rv = fread(&q2, 1, sizeof(short), file_replay);
    if (rv != sizeof(short)) break; 
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

void RspDuo::validate() {
    // validate decimation
    if (nDecimation != 1 && nDecimation != 2 && nDecimation != 4 &&
        nDecimation != 8 && nDecimation != 16 && nDecimation != 32) {
        std::cerr << "Error: Decimation must be in {1, 2, 4, 8, 16, 32}" << std::endl;
        exit(1);
    }

    // validate fc
    if (fc < 1 || fc > MAX_FREQUENCY_NR) {
        std::cerr << "Error: Frequency must be between 1 and " << 
          MAX_FREQUENCY_NR << std::endl;
        exit(1);
    }

    // validate agc
    if (agc_bandwidth_nr != 0 && agc_bandwidth_nr != 5 && 
      agc_bandwidth_nr != 50 && agc_bandwidth_nr != 100) {
        std::cerr << "Error: AGC bandwidth must be in {0, 5, 50, 100}" << std::endl;
        exit(1);
    }
    if (agc_set_point_nr > 0 || agc_set_point_nr < MIN_AGC_SET_POINT_NR) {
        std::cerr << "Error: AGC set point must be between " << 
          MIN_AGC_SET_POINT_NR << " and 0" << std::endl;
        exit(1);
    }

    // validate LNA
    if (gain_reduction_nr < MIN_GAIN_REDUCTION_NR || gain_reduction_nr > MAX_GAIN_REDUCTION_NR) {
        std::cerr << "Error: Gain reduction must be between " << MIN_GAIN_REDUCTION_NR << " and " << MAX_GAIN_REDUCTION_NR << std::endl;
        exit(1);
    }
    if (lna_state_nr < 1 || lna_state_nr > MAX_LNA_STATE_NR) {
        std::cerr << "Error: LNA state must be between 1 and " << MAX_LNA_STATE_NR << std::endl;
        exit(1);
    }

    // validate notch filters

    // print them out
    std::cerr << "[RspDuo] Print config" << std::endl;
    std::cerr << "fc (Hz)                       : " << fc << std::endl;
    std::cerr << "fs (Hz)                       : " << fs << std::endl;
    std::cerr << "file                          : " << file.c_str() << std::endl;
    std::cerr << "agc_bandwidth_nr (Hz)         : " << agc_bandwidth_nr << std::endl;
    std::cerr << "agc_set_point_nr (dBfs)       : " << agc_set_point_nr << std::endl;
    std::cerr << "gain_reduction_nr (dB)        : " << gain_reduction_nr << std::endl;
    std::cerr << "lna_state_nr                  : " << lna_state_nr << std::endl;
    std::cerr << "N_DECIMATION                  : " << nDecimation << std::endl;
    std::cerr << "rf_notch_fg                   : " << (rf_notch_fg ? "true" : "false") << std::endl;
    std::cerr << "dab_notch_fg                  : " << (dab_notch_fg ? "true" : "false") << std::endl;
    std::cerr << "usb_bulk_fg                   : " << (usb_bulk_fg ? "true" : "false") << std::endl;
    std::cerr << "stats_fg                      : " << (stats_fg ? "true" : "false") << std::endl;
    std::cerr << "\n";
}

void RspDuo::open_api()
{
  float ver = 0.0;
  // open the sdrplay api
  if ((err = sdrplay_api_Open()) != sdrplay_api_Success)
  {
    std::cerr << "Error: API open failed " << 
      sdrplay_api_GetErrorString(err) << std::endl;
    exit(1);
  }
  // check api versions match
  if ((err = sdrplay_api_ApiVersion(&ver)) != sdrplay_api_Success)
  {
    std::cerr << "Error: Set API version failed " << 
      sdrplay_api_GetErrorString(err) << std::endl;
    sdrplay_api_Close();
    exit(1);
  }
  if (ver != SDRPLAY_API_VERSION)
  {
    std::cerr << "Error: API versions do not match, local= " << SDRPLAY_API_VERSION << "API= " << ver << std::endl;
    sdrplay_api_Close();
    exit(1);
  }
}

void RspDuo::get_device()
{
  unsigned int i;
  unsigned int ndev;
  unsigned int chosenIdx = 0;

  // lock api while device selection is performed
  if ((err = sdrplay_api_LockDeviceApi()) != sdrplay_api_Success)
  {
    std::cerr << "Error: Lock API during device selection failed " << 
      sdrplay_api_GetErrorString(err) << std::endl;
    sdrplay_api_Close();
    exit(1);
  }

  // fetch list of available devices
  if ((err = sdrplay_api_GetDevices(devs, &ndev, 
    sizeof(devs) / sizeof(sdrplay_api_DeviceT))) != sdrplay_api_Success)
  {
    std::cerr << "Error: sdrplay_api_GetDevices failed " << 
      sdrplay_api_GetErrorString(err) << std::endl;
    sdrplay_api_UnlockDeviceApi();
    sdrplay_api_Close();
    exit(1);
  }

  std::cerr << "[RspDuo] MaxDevs=" << sizeof(devs) / 
    sizeof(sdrplay_api_DeviceT) << " NumDevs=" << ndev << std::endl;

  if (ndev == 0)
  {
    std::cerr << "Error: No devices found" << std::endl;
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
      break;
    }
  }

  if (i == ndev)
  {
    std::cerr << "Error: Could not find RSPduo device to open" << std::endl;
    sdrplay_api_UnlockDeviceApi();
    sdrplay_api_Close();
    exit(1);
  }

  chosenDevice = &devs[chosenIdx];
  chosenDevice->tuner = sdrplay_api_Tuner_Both;
  chosenDevice->rspDuoMode = sdrplay_api_RspDuoMode_Dual_Tuner;

  std::cerr << "[RspDuo] Device ID " << chosenIdx << std::endl;
  std::cerr << "[RspDuo] Serial Number " << devs[i].SerNo << std::endl;
  std::cerr << "[RspDuo] Hardware Version " << std::to_string(devs[i].hwVer) << std::endl;
  std::cerr << "[RspDuo] Tuner " << std::hex << chosenDevice->tuner << std::dec << std::endl;
  std::cerr << "[RspDuo] RspDuoMode " << std::hex << chosenDevice->rspDuoMode << std::dec << std::endl;

  // select chosen device
  if ((err = sdrplay_api_SelectDevice(chosenDevice)) != sdrplay_api_Success)
  {
    std::cerr << "Error: Select device failed " << 
      sdrplay_api_GetErrorString(err) << std::endl;
    sdrplay_api_UnlockDeviceApi();
    sdrplay_api_Close();
    exit(1);
  }

  // unlock api now that device is selected
  if ((err = sdrplay_api_UnlockDeviceApi()) != sdrplay_api_Success)
  {
    std::cerr << "Error: Unlock device API failed " << 
      sdrplay_api_GetErrorString(err) << std::endl;
    sdrplay_api_Close();
    exit(1);
  }

  // enable debug logging output
  if ((err = sdrplay_api_DebugEnable(chosenDevice->dev, sdrplay_api_DbgLvl_Verbose)) != sdrplay_api_Success)
  {
    std::cerr << "Error: Debug enable failed " << 
      sdrplay_api_GetErrorString(err) << std::endl;
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
    std::cout << "Error: sdrplay_api_GetDeviceParams failed " + 
      std::string(sdrplay_api_GetErrorString(err)) << std::endl;
    sdrplay_api_Close();
    exit(1);
  }

  // check for NULL pointer before changing settings
  if (deviceParams == NULL)
  {
    std::cout << "Error: Device parameters pointer is null" << std::endl;
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
    std::cerr << "Error: Channel parameters pointer is null" << std::endl;
    sdrplay_api_Close();
    exit(1);
  }

  // set center frequency
  chParams->tunerParams.rfFreq.rfHz = fc;

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
  chParams->ctrlParams.decimation.decimationFactor = nDecimation;
  chParams->tunerParams.ifType = ifType;
  chParams->tunerParams.bwType = bwType;

  // configure notch filters
  chParams->rspDuoTunerParams.rfNotchEnable = rf_notch_fg;
  chParams->rspDuoTunerParams.rfDabNotchEnable = dab_notch_fg;

  // assign callback functions to be passed to sdrplay_api_Init()
  cbFns.StreamACbFn = _stream_a_callback;
  cbFns.StreamBCbFn = _stream_b_callback;
  cbFns.EventCbFn = _event_callback;

  return;
}

void RspDuo::stream_a_callback(short *xi, short *xq, 
sdrplay_api_StreamCbParamsT *params, unsigned int numSamples, 
unsigned int reset, void *cbContext)
{
  unsigned int i = 0;
  unsigned int j = 0;

  // process stream callback data
  buffer_16_ar = (short int *)malloc(numSamples * 4 * sizeof(short));

  if (buffer_16_ar == NULL)
  {
    std::cout << "Error: stream_a_callback, malloc failed" << std::endl;
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
  if (stats_fg)
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

void RspDuo::stream_b_callback(short *xi, short *xq, 
sdrplay_api_StreamCbParamsT *params, unsigned int numSamples, 
unsigned int reset, void *cbContext)
{
  unsigned int i = 0;
  unsigned int j = 0;

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
  for (i = 0; i < numSamples*4; i+=4)
  {
    buffer1->push_back({(double)buffer_16_ar[i], (double)buffer_16_ar[i+1]});
    buffer2->push_back({(double)buffer_16_ar[i+2], (double)buffer_16_ar[i+3]});
  }
  buffer1->unlock();
  buffer2->unlock();

  // write data to file
  if (*capture_fg)
  {
    saveIqFileLocal->write(reinterpret_cast<char*>(buffer_16_ar), 
      sizeof(short) * numSamples * 4);
    
    if (!(*saveIqFileLocal))
    {
      std::cout << "Error: stream_b_callback, not enough samples received" << std::endl;
      free(buffer_16_ar);
      run_fg = false;
      return;
    }
  }

  free(buffer_16_ar);

  // find max for stats
  if (stats_fg)
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

void RspDuo::event_callback(sdrplay_api_EventT eventId, 
sdrplay_api_TunerSelectT tuner, sdrplay_api_EventParamsT *params, 
void *cbContext)
{
  std::string tuner_str = (tuner == sdrplay_api_Tuner_A) ? 
    "sdrplay_api_Tuner_A" : "sdrplay_api_Tuner_B";
  switch (eventId)
  {
  case sdrplay_api_GainChange:
    std::cerr << "[RspDuo] Gain change, tuner=" << tuner_str << " ";
    std::cerr << "gRdB=" << params->gainParams.gRdB << " ";
    std::cerr << "lnaGRdB=" << params->gainParams.lnaGRdB << " ";
    std::cerr << "systemGain=" << params->gainParams.currGain << std::endl;
    break;

  case sdrplay_api_PowerOverloadChange:
    std::cerr << "[RspDuo] PowerOverloadChange, tuner=" << tuner_str << " ";
    std::cerr << "powerOverloadChangeType=" << 
      ((params->powerOverloadParams.powerOverloadChangeType 
      == sdrplay_api_Overload_Detected) ? "sdrplay_api_Overload_Detected" : 
      "sdrplay_api_Overload_Corrected") << std::endl;
    // send update message to acknowledge power overload message received
    sdrplay_api_Update(chosenDevice->dev, tuner, 
      sdrplay_api_Update_Ctrl_OverloadMsgAck, sdrplay_api_Update_Ext1_None);
    break;

  case sdrplay_api_DeviceRemoved:
    std::cerr << "[RspDuo] Device removed" << std::endl;
    break;

  default:
    std::cerr << "[RspDuo] Unknown event " << eventId << std::endl;
    break;
  }
}

void RspDuo::initialise_device()
{
  if ((err = sdrplay_api_Init(chosenDevice->dev, &cbFns, NULL)) != sdrplay_api_Success)
  {
    std::cerr << "Error: sdrplay_api_Init failed " << 
      sdrplay_api_GetErrorString(err) << std::endl;
    sdrplay_api_Close();
    exit(1);
  }
}

void RspDuo::uninitialise_device()
{
  if ((err = sdrplay_api_Uninit(chosenDevice->dev)) != sdrplay_api_Success)
  {
    std::cerr << "Error: sdrplay_api_Uninit failed " << 
      sdrplay_api_GetErrorString(err) << std::endl;
    sdrplay_api_Close();
    exit(1);
  }
  sdrplay_api_ReleaseDevice(chosenDevice);
  sdrplay_api_Close();
}
