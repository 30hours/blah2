#include "Source.h"

#include <iostream>

Source::Source()
{
  
}

// constructor
Source::Source(std::string _type, uint32_t _fc, uint32_t _fs, 
    std::string _path, bool *_saveIq)
{
  type = _type;
  fc = _fc;
  fs = _fs;
  path = _path;
  saveIq = _saveIq;
}
