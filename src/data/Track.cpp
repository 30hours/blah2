#include "Track.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filewritestream.h"

const uint64_t Track::MAX_INDEX = 65535;
const std::string Track::STATE_ACTIVE = "ACTIVE";
const std::string Track::STATE_TENTATIVE = "TENTATIVE";
const std::string Track::STATE_COASTING = "COASTING";
const std::string Track::STATE_ASSOCIATED = "ASSOCIATED";

// constructor
Track::Track()
{
  iNext = 0;
}

Track::~Track()
{
}

std::string Track::uint2hex(uint64_t number)
{
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << std::uppercase << std::hex << number;
    return oss.str();
}

void Track::set_state(uint64_t index, std::string _state)
{
  state.at(index).push_back(_state);
}

void Track::set_current(uint64_t index, Detection smoothed)
{
  current.at(index) = smoothed;
  associated.at(index).push_back(smoothed);
}

void Track::set_acceleration(uint64_t index, double _acceleration)
{
  acceleration.at(index) = _acceleration;
}

void Track::set_nInactive(uint64_t index, uint64_t n)
{
  nInactive.at(index) = n;
}

uint64_t Track::get_nActive()
{
  uint64_t n = 0;
  for (size_t i = 0; i < id.size(); i++)
  {
    if (get_state(i) == STATE_ACTIVE)
    {
      n++;
    }
  }
  return n;
}

uint64_t Track::get_nTentative()
{
  uint64_t n = 0;
  for (size_t i = 0; i < id.size(); i++)
  {
    if (get_state(i) == STATE_TENTATIVE)
    {
      n++;
    }
  }
  return n;
}

uint64_t Track::get_n()
{
  return id.size();
}

Detection Track::get_current(uint64_t index)
{
  return current.at(index);
}

double Track::get_acceleration(uint64_t index)
{
  return acceleration.at(index);
}

std::string Track::get_state(uint64_t index)
{
  return state.at(index).at(state.at(index).size()-1);
}

uint64_t Track::get_nInactive(uint64_t index)
{
  return nInactive.at(index);
}

uint64_t Track::add(Detection initial)
{
  id.push_back(uint2hex(iNext));
  std::vector<std::string> _state;
  _state.push_back(STATE_TENTATIVE);
  state.push_back(_state);
  current.push_back(initial);
  acceleration.push_back(0);
  std::vector<Detection> _associated;
  _associated.push_back(initial);
  associated.push_back(_associated);
  nInactive.push_back(0);
  iNext++;
  if (iNext >= MAX_INDEX)
  {
    iNext = 0;
  }
  return id.size()-1;
}

void Track::promote(uint64_t index, uint32_t m, uint32_t n)
{
  if (state.at(index).size() >= n)
  {
    uint32_t _m = 0;
    for (size_t i = state.at(index).size()-n; i < state.at(index).size(); i++) 
    {
      if (state.at(index).at(i) == STATE_ACTIVE || 
        state.at(index).at(i) == STATE_ASSOCIATED)
      {
        _m++;
      }
    }
    // promote track to ACTIVE if passes test
    if (_m >= m)
    {
      state.at(index).at(state.at(index).size()-1) = STATE_ACTIVE;
    }
  }
}

void Track::remove(uint64_t index)
{
  id.erase(id.begin() + index);
  state.erase(state.begin() + index);
  current.erase(current.begin() + index);
  acceleration.erase(acceleration.begin() + index);
  associated.erase(associated.begin() + index);
}

std::string Track::to_json(uint64_t timestamp)
{
  rapidjson::Document document;
  document.SetObject();
  rapidjson::Document::AllocatorType &allocator = document.GetAllocator();

  // store array for non-tentative tracks
  rapidjson::Value arrayId(rapidjson::kArrayType);
  rapidjson::Value value;
  for (int i = 0; i < get_n(); i++)
  {
    if (get_state(i) != STATE_TENTATIVE)
    {
      value = rapidjson::StringRef(id.at(i).c_str());
      arrayId.PushBack(value, allocator);
    }
  }

  document.AddMember("timestamp", timestamp, allocator);
  document.AddMember("n", get_n(), allocator);
  document.AddMember("nActive", get_nActive(), allocator);
  document.AddMember("nTentative", get_nTentative(), allocator);
  document.AddMember("id", arrayId, allocator);
  
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  writer.SetMaxDecimalPlaces(2);
  document.Accept(writer);

  return strbuf.GetString();
}