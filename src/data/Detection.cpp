#include "Detection.h"
#include "data/meta/Constants.h"
#include <iostream>
#include <cstdlib>
#include <chrono>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filewritestream.h"

// constructor
Detection::Detection(std::vector<double> _delay, std::vector<double> _doppler, std::vector<double> _snr)
{
  delay = _delay;
  doppler = _doppler;
  snr = _snr;
}

Detection::Detection(double _delay, double _doppler, double _snr)
{
  delay.push_back(_delay);
  doppler.push_back(_doppler);
  snr.push_back(_snr);
}

std::vector<double> Detection::get_delay()
{
  return delay;
}

std::vector<double> Detection::get_doppler()
{
  return doppler;
}

std::vector<double> Detection::get_snr()
{
  return snr;
}

size_t Detection::get_nDetections()
{
  return delay.size();
}

std::string Detection::to_json(uint64_t timestamp)
{
  rapidjson::Document document;
  document.SetObject();
  rapidjson::Document::AllocatorType &allocator = document.GetAllocator();

  // store delay array
  rapidjson::Value arrayDelay(rapidjson::kArrayType);
  for (size_t i = 0; i < get_nDetections(); i++)
  {
    arrayDelay.PushBack(delay[i], allocator);
  }

  // store Doppler array
  rapidjson::Value arrayDoppler(rapidjson::kArrayType);
  for (size_t i = 0; i < get_nDetections(); i++)
  {
    arrayDoppler.PushBack(doppler[i], allocator);
  }

  // store snr array
  rapidjson::Value arraySnr(rapidjson::kArrayType);
  for (size_t i = 0; i < get_nDetections(); i++)
  {
    arraySnr.PushBack(snr[i], allocator);
  }

  document.AddMember("timestamp", timestamp, allocator);
  document.AddMember("delay", arrayDelay, allocator);
  document.AddMember("doppler", arrayDoppler, allocator);
  document.AddMember("snr", arraySnr, allocator);
  
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  writer.SetMaxDecimalPlaces(2);
  document.Accept(writer);

  return strbuf.GetString();
}

std::string Detection::delay_bin_to_km(std::string json, uint32_t fs)
{
  rapidjson::Document document;
  document.SetObject();
  rapidjson::Document::AllocatorType &allocator = document.GetAllocator();
  document.Parse(json.c_str());

  document["delay"].Clear();
  for (size_t i = 0; i < delay.size(); i++)
  {
    document["delay"].PushBack(1.0*delay[i]*(Constants::c/(double)fs)/1000, allocator);
  }

  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  writer.SetMaxDecimalPlaces(2);
  document.Accept(writer);

  return strbuf.GetString();
}

bool Detection::save(std::string _json, std::string filename)
{
  using namespace rapidjson;

  rapidjson::Document document;

  // create file if it doesn't exist
  if (FILE *fp = fopen(filename.c_str(), "r"); !fp)
  {
    if (fp = fopen(filename.c_str(), "w"); !fp)
      return false;
    fputs("[]", fp);
    fclose(fp);
  }

  // add the document to the file
  if (FILE *fp = fopen(filename.c_str(), "rb+"); fp)
  {
    // check if first is [
    std::fseek(fp, 0, SEEK_SET);
    if (getc(fp) != '[')
    {
      std::fclose(fp);
      return false;
    }

    // is array empty?
    bool isEmpty = false;
    if (getc(fp) == ']')
      isEmpty = true;

    // check if last is ]
    std::fseek(fp, -1, SEEK_END);
    if (getc(fp) != ']')
    {
      std::fclose(fp);
      return false;
    }

    // replace ] by ,
    fseek(fp, -1, SEEK_END);
    if (!isEmpty)
      fputc(',', fp);

    // add json element
    fwrite(_json.c_str(), sizeof(char), _json.length(), fp);

    // close the array
    std::fputc(']', fp);
    fclose(fp);
    return true;
  }
  return false;
}
