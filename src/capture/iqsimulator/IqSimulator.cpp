#include "IqSimulator.h"

// constructor
IqSimulator::IqSimulator(std::string _type, uint32_t _fc, uint32_t _fs,
                         std::string _path, bool *_saveIq,
                         uint32_t _n_min = 1000,
                         std::string _falseTargetsConfigFilePath,
                         std::string _configFilePath)
    : Source(_type, _fc, _fs, _path, _saveIq)
{
    n_min = _n_min;
    u_int64_t total_samples = 0;
    false_targets_config_file_path = _falseTargetsConfigFilePath;
    config_file_path = _configFilePath;
}

void IqSimulator::start()
{
}

void IqSimulator::stop()
{
}

void IqSimulator::process(IqData *buffer1, IqData *buffer2)
{
    const u_int32_t samples_per_iteration = 1000;

    TgtGen false_targets = TgtGen(false_targets_config_file_path, config_file_path, fs, fc);
    while (true)
    {
        uint32_t n_start = buffer1->get_length();
        if (n_start < n_min)
        {

            // create a random number generator
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(-(2 ^ 14), 2 ^ 14);

            buffer1->lock();
            buffer2->lock();
            for (uint16_t i = 0; i < samples_per_iteration; i++)
            {

                buffer1->push_back({(double)dis(gen), (double)dis(gen)});
                try
                {
                    std::complex<double> response = false_targets.process(buffer1);
                    response += std::complex<double>((double)dis(gen), (double)dis(gen));
                    buffer2->push_back(response);
                }
                catch (const std::exception &e)
                {
                    buffer2->push_back({(double)dis(gen), (double)dis(gen)});
                }
            }
            total_samples += samples_per_iteration;
            buffer1->unlock();
            buffer2->unlock();
        }
    }
}

void IqSimulator::replay(IqData *buffer1, IqData *buffer2, std::string file, bool loop)
{
}