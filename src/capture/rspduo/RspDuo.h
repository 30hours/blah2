#ifndef RSPDUO_H
#define RSPDUO_H

#include "sdrplay_api.h"
#include <stdint.h>
#include <string>
#include <IqData.h>

#define BUFFER_SIZE_NR 1024 /* standard size of buffers  */

// https://stackoverflow.com/questions/63768893/pointer-problem-using-functions-from-non-object-api-in-objects?rq=1

class RspDuo
{
private:
  uint32_t fc;           // frequency (Hz)
  int chunk_time_nr;     // chunk time of recording (s)
  int agc_bandwidth_nr;  // agc bandwidth (Hz)
  int agc_set_point_nr;  // agc set point (dBfs)
  int gain_reduction_nr; // gain reduction (dB)
  int lna_state_nr;      // lna state
  int nDecimation;       // decimation factor
  bool rf_notch_fg;      // MW and FM notch filters
  bool dab_notch_fg;     // DAB notch filter
  bool usb_bulk_fg;      // usb bulk transfer mode
  bool small_verbose_fg; // debugging
  bool more_verbose_fg;  // debugging
  std::string path;      // file path
  bool capture;          // flag to capture
  static const double MAX_FREQUENCY_NR;
  static const uint8_t DEF_DECIMATION_NR;
  static const int DEF_WAIT_TIME_NR;        // default wait time before recording
  static const int DEF_CHUNK_TIME_NR;       // default chunk time of recording
  static const int MAX_RUN_TIME_NR;         // max run time of recording
  static const int DEF_AGC_BANDWIDTH_NR;    // default agc bandwidth
  static const int MIN_AGC_SET_POINT_NR;    // min agc set point
  static const int DEF_AGC_SET_POINT_NR;    // default agc set point
  static const int MIN_GAIN_REDUCTION_NR;   // min gain reduction
  static const int DEF_GAIN_REDUCTION_NR;   // default gain reduction
  static const int MAX_GAIN_REDUCTION_NR;   // max gain reduction
  static const int DEF_LNA_STATE_NR;        // default lna state
  static const int MAX_LNA_STATE_NR;        // max lna state
  static const int DEF_SAMPLE_FREQUENCY_NR; // default sample frequency
  static const int DEF_SAMPLE_RATE_NR;      // default sample rate

  void validate();
  void open_api();
  void get_device();
  void set_device_parameters();
  static void _stream_a_callback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, unsigned int numSamples, unsigned int reset, void *cbContext)
  {
    static_cast<RspDuo *>(cbContext)->stream_a_callback(xi, xq, params, numSamples, reset, cbContext);
  };
  static void _stream_b_callback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, unsigned int numSamples, unsigned int reset, void *cbContext)
  {
    static_cast<RspDuo *>(cbContext)->stream_b_callback(xi, xq, params, numSamples, reset, cbContext);
  };
  static void _event_callback(sdrplay_api_EventT eventId, sdrplay_api_TunerSelectT tuner, sdrplay_api_EventParamsT *params, void *cbContext)
  {
    static_cast<RspDuo *>(cbContext)->event_callback(eventId, tuner, params, cbContext);
  };
  void stream_a_callback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, unsigned int numSamples, unsigned int reset, void *cbContext);
  void stream_b_callback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, unsigned int numSamples, unsigned int reset, void *cbContext);
  void event_callback(sdrplay_api_EventT eventId, sdrplay_api_TunerSelectT tuner, sdrplay_api_EventParamsT *params, void *cbContext);
  void initialise_device();
  void uninitialise_device();
  void finish();

public:
  RspDuo(uint32_t fc, std::string path);
  std::string set_file(std::string path);
  void start();
  void stop();
  void process(IqData *buffer1, IqData *buffer2);
  void replay(IqData *buffer1, IqData *buffer2, std::string file, bool loop);
  void open_file();
  void close_file();
  void set_capture(bool capture);
  bool get_capture();
};

#endif