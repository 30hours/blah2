#include "TgtGen.h"

// this is straight up copied from blah2.cpp, but I don't know the best way to access that function here.
// edit: put it in utilities?
std::string ryml_get_file_copy(const char *filename);

// constants
const std::string TgtGen::VALID_TYPE[1] = {"static"};
const std::string TgtGen::VALID_STATE[1] = {"active"};

// constructor
TgtGen::TgtGen(std::string configPath, uint32_t fs)
{
    // Read in the config file
    std::string config = ryml_get_file_copy(configPath.c_str());
    ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(config));

    // Create a FalseTarget object for each target in the config file
    for (auto target_node : tree["targets"].children())
    {
        if (target_node["state"].val() == VALID_STATE[0])
        {
            try
            {
                targets.push_back(FalseTarget(target_node, fs));
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
        }
    }
}

std::complex<double> TgtGen::process(IqData *ref_buffer)
{
    std::complex<double> response = std::complex<double>(0, 0);
    // loop through each target
    for (auto target : targets)
    {
        response += target.process(ref_buffer);
    }

    return response;
}

std::string FalseTarget::get_type()
{
    return type;
}

double FalseTarget::get_range()
{
    return range;
}

double FalseTarget::get_doppler()
{
    return doppler;
}

double FalseTarget::get_rcs()
{
    return rcs;
}

double FalseTarget::get_delay()
{
    return delay;
}

u_int32_t FalseTarget::get_id()
{
    return id;
}

FalseTarget::FalseTarget(c4::yml::NodeRef target_node, uint32_t _fs)
{

    target_node["id"] >> id;
    target_node["type"] >> type;
    fs = _fs;

    if (type == TgtGen::VALID_TYPE[0])
    {
        target_node["location"]["range"] >> range;
        delay = range / 3e8;
        delay_samples = delay * fs;
        target_node["velocity"]["doppler"] >> doppler;
        target_node["rcs"] >> rcs;
    }
    else
    {
        throw std::invalid_argument("Invalid target type");
    }
}

std::complex<double> FalseTarget::process(IqData *buffer)
{
    uint32_t buffer_length = buffer->get_length();
    std::complex<double> response = 0;
    try
    {
        response = Conversions::db2lin(rcs) * buffer->get_sample(buffer_length - delay_samples);
        response *= std::exp(std::polar<double>(1, 2 * M_PI * doppler * buffer_length / fs));
    }
    catch (const std::exception &e)
    {
    }

    return response;
}

std::string ryml_get_file_copy(const char *filename)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (!in)
    {
        std::cerr << "could not open " << filename << std::endl;
        exit(1);
    }
    std::ostringstream contents;
    contents << in.rdbuf();
    return contents.str();
}