#include "Timing.h"
#include <iostream>
#include <cstdlib>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filewritestream.h"

// constructor
Timing::Timing(uint64_t _tStart)
{
  tStart = _tStart;
  n = 0;
}

void Timing::update(uint64_t _tNow, std::vector<double> _time, std::vector<std::string> _name)
{
  n = n + 1;
  tNow = _tNow;
  time = _time;
  name = _name;
  uptime = _tNow-tStart;
}

std::string Timing::to_json()
{
  rapidjson::Document document;
  document.SetObject();
  rapidjson::Document::AllocatorType &allocator = document.GetAllocator();

  document.AddMember("timestamp", tNow, allocator);
  document.AddMember("nCpi", n, allocator);
  document.AddMember("uptime_s", uptime/1000.0, allocator);
  document.AddMember("uptime_days", uptime/1000.0/60/60/24, allocator);
  rapidjson::Value name_value;
  for (size_t i = 0; i < time.size(); i++)
  {
    name_value = rapidjson::StringRef(name[i].c_str());
    document.AddMember(name_value, time[i], allocator);
  }

  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  writer.SetMaxDecimalPlaces(2);
  document.Accept(writer);

  return strbuf.GetString();
}

bool Timing::save(std::string _json, std::string filename)
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
