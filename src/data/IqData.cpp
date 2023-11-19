#include "IqData.h"
#include <iostream>
#include <cstdlib>

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