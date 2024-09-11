#include "IqData.h"
#include <iostream>
#include <cstdlib>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filewritestream.h"

// constructor
IqData::IqData(uint32_t _n)
{
  n = _n;
  data = new std::deque<std::complex<double>>;
}

uint32_t IqData::get_n()
{
  return n;
}

uint32_t IqData::get_length()
{
  return data->size();
}

void IqData::lock()
{
  mutex_lock.lock();
}

void IqData::unlock()
{
  mutex_lock.unlock();
}

std::deque<std::complex<double>> IqData::get_data()
{
  return *data;
}

void IqData::push_back(std::complex<double> sample)
{
  if (data->size() < n)
  {
    data->push_back(sample);
  }
  else
  {
    data->pop_front();
    data->push_back(sample);
  }
}

std::complex<double> IqData::pop_front()
{
  if (data->empty()) {
    throw std::runtime_error("Attempting to pop from an empty deque");
  }
  std::complex<double> sample = data->front();
  data->pop_front();
  return sample;
}
void IqData::print()
{
  int n = data->size();
  std::cout << data->size() << std::endl;
  for (int i = 0; i < n; i++)
  {
    std::cout << data->front() << std::endl;
    data->pop_front();
  }
}

void IqData::clear()
{
  while (!data->empty())
  {
    data->pop_front();
  }
}

void IqData::update_spectrum(std::vector<std::complex<double>> _spectrum)
{
  spectrum = _spectrum;
}

void IqData::update_frequency(std::vector<double> _frequency)
{
  frequency = _frequency;
}

std::string IqData::to_json(uint64_t timestamp)
{
  rapidjson::Document document;
  document.SetObject();
  rapidjson::Document::AllocatorType &allocator = document.GetAllocator();

  // store frequency array
  rapidjson::Value arrayFrequency(rapidjson::kArrayType);
  for (size_t i = 0; i < frequency.size(); i++)
  {
    arrayFrequency.PushBack(frequency[i], allocator);
  }

  // store spectrum array
  rapidjson::Value arraySpectrum(rapidjson::kArrayType);
  for (size_t i = 0; i < spectrum.size(); i++)
  {
    arraySpectrum.PushBack(10 * std::log10(std::abs(spectrum[i])), allocator);
  }

  document.AddMember("timestamp", timestamp, allocator);
  document.AddMember("min", min, allocator);
  document.AddMember("max", max, allocator);
  document.AddMember("mean", mean, allocator);
  document.AddMember("frequency", arrayFrequency, allocator);
  document.AddMember("spectrum", arraySpectrum, allocator);

  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  writer.SetMaxDecimalPlaces(2);
  document.Accept(writer);

  return strbuf.GetString();
}