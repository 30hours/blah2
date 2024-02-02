#include "IqSimulator.h"

#include <string.h>
#include <iostream>
#include <vector>
#include <complex>
#include <random>

// constructor
IqSimulator::IqSimulator(std::string _type, uint32_t _fc, uint32_t _fs,
                         std::string _path, bool *_saveIq, uint32_t _n_min = 1000)
    : Source(_type, _fc, _fs, _path, _saveIq)
{
    n_min = _n_min;
}

void IqSimulator::start()
{
}

void IqSimulator::stop()
{
}

void IqSimulator::process(IqData *buffer1, IqData *buffer2)
{
    while (true)
    {
        if (buffer1->get_length() < n_min)
        {
            // create a random number generator
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(-(2 ^ 14), 2 ^ 14);

            buffer1->lock();
            buffer2->lock();
            for (int i = 0; i < 1000; i++)
            {
                buffer1->push_back({(double)dis(gen), (double)dis(gen)});
                buffer2->push_back({(double)dis(gen), (double)dis(gen)});
            }
            buffer1->unlock();
            buffer2->unlock();
        }
    }
}

void IqSimulator::replay(IqData *buffer1, IqData *buffer2, std::string file, bool loop)
{
}