#ifndef CAPTURE_H
#define CAPTURE_H

#include <string>
#include <IqData.h>

class Capture
{
private:
  static const std::string VALID_TYPE[2];
  bool replay;
  bool loop;
  std::string file;
public:
  std::string type;
  uint32_t fs;
  uint32_t fc;
  bool saveIq;
  std::string path;
  Capture(std::string type, uint32_t fs, uint32_t fc, std::string path);
  void process(IqData *buffer1, IqData *buffer2);
  void set_replay(bool loop, std::string file);

};

#endif