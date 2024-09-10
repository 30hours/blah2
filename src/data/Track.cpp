#include "Track.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <stdexcept>

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

uint64_t Track::get_nState(std::string _state)
{
  uint64_t n = 0;
  for (size_t i = 0; i < id.size(); i++)
  {
    if (get_state(i) == _state)
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

    // promote track to ACTIVE if passes threshold
    if (_m >= m)
    {
      state.at(index).at(state.at(index).size()-1) = STATE_ACTIVE;
    }
  }
}

void Track::remove(uint64_t index)
{
  // Check if the index is within bounds for each vector
  if (index < id.size()) {
    id.erase(id.begin() + index);
  } else {
    // Throw an exception if the index is out of bounds
    throw std::out_of_range("Index out of bounds for 'id' vector");
  }

  if (index < state.size()) {
    state.erase(state.begin() + index);
  } else {
    throw std::out_of_range("Index out of bounds for 'state' vector");
  }

  if (index < current.size()) {
    current.erase(current.begin() + index);
  } else {
    throw std::out_of_range("Index out of bounds for 'current' vector");
  }

  if (index < acceleration.size()) {
    acceleration.erase(acceleration.begin() + index);
  } else {
    throw std::out_of_range("Index out of bounds for 'acceleration' vector");
  }

  if (index < associated.size()) {
    associated.erase(associated.begin() + index);
  } else {
    throw std::out_of_range("Index out of bounds for 'associated' vector");
  }
}

std::string Track::to_json(uint64_t timestamp)
{
  rapidjson::Document document;
  document.SetObject();
  rapidjson::Document::AllocatorType &allocator = document.GetAllocator();

  // store track data
  rapidjson::Value dataArray(rapidjson::kArrayType);
  for (uint64_t i = 0; i < get_n(); i++)
  {
    if (get_state(i) != STATE_TENTATIVE)
    {
      rapidjson::Value object1(rapidjson::kObjectType);
      object1.AddMember("id", rapidjson::Value(id.at(i).c_str(), 
        document.GetAllocator()).Move(), document.GetAllocator());
      object1.AddMember("state", rapidjson::Value(
        state.at(i).at(state.at(i).size()-1).c_str(), 
        document.GetAllocator()).Move(), document.GetAllocator());
      object1.AddMember("delay", 
        current.at(i).get_delay().at(0),
        document.GetAllocator());
      object1.AddMember("doppler", 
        current.at(i).get_doppler().at(0), 
        document.GetAllocator());
      object1.AddMember("acceleration", 
        acceleration.at(i), document.GetAllocator());
      object1.AddMember("n", associated.at(i).size(), 
        document.GetAllocator());
      rapidjson::Value associatedDelay(rapidjson::kArrayType);
      rapidjson::Value associatedDoppler(rapidjson::kArrayType);
      rapidjson::Value associatedState(rapidjson::kArrayType);
      for (size_t j = 0; j < associated.at(i).size(); j++)
      {
        associatedDelay.PushBack(associated.at(i).at(j).get_delay().at(0), 
          document.GetAllocator());
        associatedDoppler.PushBack(associated.at(i).at(j).get_doppler().at(0), 
          document.GetAllocator());
        associatedState.PushBack(rapidjson::Value(state.at(i).at(j).c_str(), 
          document.GetAllocator()).Move(), document.GetAllocator());
      }
      object1.AddMember("associated_delay", 
        associatedDelay, document.GetAllocator());
      object1.AddMember("associated_doppler", 
        associatedDoppler, document.GetAllocator());
      object1.AddMember("associated_state", 
        associatedState, document.GetAllocator());
      dataArray.PushBack(object1, document.GetAllocator());
    }
  }

  document.AddMember("timestamp", timestamp, allocator);
  document.AddMember("n", get_n(), allocator);
  document.AddMember("nTentative", get_nState(STATE_TENTATIVE), allocator);
  document.AddMember("nAssociated", get_nState(STATE_ASSOCIATED), allocator);
  document.AddMember("nActive", get_nState(STATE_ACTIVE), allocator);
  document.AddMember("nCoasting", get_nState(STATE_COASTING), allocator);
  document.AddMember("data", dataArray, document.GetAllocator());
  
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  writer.SetMaxDecimalPlaces(2);
  document.Accept(writer);

  return strbuf.GetString();
}