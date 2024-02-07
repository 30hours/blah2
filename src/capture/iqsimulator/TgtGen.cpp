#include "TgtGen.h"

// this is straight up copied from blah2.cpp, but I don't know the best way to access that function here.
// edit: put it in utilities?
std::string ryml_get_file_copy(const char *filename);

// constants
const std::string TgtGen::VALID_TYPE[2] = {"static", "moving_radar"};
const std::string TgtGen::VALID_STATE[1] = {"active"};

// constructor
TgtGen::TgtGen(std::string false_tgt_config_path, std::string config_path, uint32_t fs, uint32_t fc)
{
    // Read in the false targets config file
    std::string config = ryml_get_file_copy(false_tgt_config_path.c_str());
    ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(config));

    // Create a FalseTarget object for each target in the config file
    for (auto target_node : tree["targets"].children())
    {
        if (target_node["state"].val() == VALID_STATE[0])
        {
            try
            {
                targets.push_back(FalseTarget(target_node, fs, fc));
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
        }
    }

    // Create the socket using details from the config file.
    config = ryml_get_file_copy(config_path.c_str());
    tree = ryml::parse_in_arena(ryml::to_csubstr(config));

    std::string ip;
    uint16_t port;

    tree["network"]["ip"] >> ip;
    tree["network"]["ports"]["falsetargets"] >> port;

    socket = new Socket(ip, port);

    sample_counter = 0;
}

std::complex<double> TgtGen::process(IqData *ref_buffer)
{
    std::complex<double> response = std::complex<double>(0, 0);
    // loop through each target
    for (auto &target : targets)
    {
        response += target.process(ref_buffer);
    }

    // output false target truth
    if (sample_counter % 100000 == 0)
    {
        rapidjson::Document document;
        document.SetObject();
        rapidjson::Document::AllocatorType &allocator = document.GetAllocator();
        rapidjson::Value json_false_targets(rapidjson::kArrayType);
        for (auto target : targets)
        {
            json_false_targets.PushBack(target.to_json(allocator), allocator);
        }

        document.AddMember("false_targets", json_false_targets, allocator);
        rapidjson::StringBuffer strbuf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
        writer.SetMaxDecimalPlaces(2);
        document.Accept(writer);

        socket->sendData(strbuf.GetString());
    }

    sample_counter++;

    return response;
}

double FalseTarget::get_range()
{
    return range;
}

void FalseTarget::set_range(double _range)
{
    range = _range;
    delay = range / Constants::c;
    delay_samples = delay * fs;
}

double FalseTarget::get_delay()
{
    return delay;
}

void FalseTarget::set_delay(double _delay)
{
    delay = _delay;
    range = delay * Constants::c;
    delay_samples = delay * fs;
}

FalseTarget::FalseTarget(c4::yml::NodeRef target_node, uint32_t _fs, uint32_t _fc)
{

    target_node["id"] >> id;
    target_node["type"] >> type;
    fs = _fs;
    fc = _fc;
    sample_counter = 0;

    if (type == TgtGen::VALID_TYPE[0])
    {
        target_node["location"]["range"] >> range;
        delay = range / Constants::c;
        delay_samples = delay * fs;
        target_node["velocity"]["doppler"] >> doppler;
        target_node["rcs"] >> rcs;
    }
    else if (type == TgtGen::VALID_TYPE[1])
    {
        target_node["location"]["range"] >> range;
        start_range = range;
        delay = range / Constants::c;
        delay_samples = delay * fs;
        target_node["velocity"]["doppler"] >> doppler;
        target_node["velocity"]["dopplerRate"] >> doppler_rate;
        target_node["rcs"] >> rcs;
    }
    else
    {
        throw std::invalid_argument("Invalid target type");
    }
}

std::complex<double> FalseTarget::process(IqData *ref_buffer)
{
    uint32_t buffer_length = ref_buffer->get_length();
    std::complex<double> response = 0;
    try
    {
        response = Conversions::db2lin(rcs) * ref_buffer->get_sample(buffer_length - delay_samples);
        response *= std::exp(std::polar<double>(1, 2 * M_PI * doppler * buffer_length / fs));

        if (type == TgtGen::VALID_TYPE[1])
        {
            double range_rate = -1 * doppler * Constants::c / 2.0 / fc;
            set_range(range + (range_rate / fs));

            // very basic PD controller
            // will need tuning for different fs
            doppler_rate += 0.0000001 * (range - start_range) / start_range; // target tends towards start_range
            // doppler_rate -= doppler / std::abs(doppler) / std::max(std::abs(doppler), 0.1) / 100; // target tends towards 0 Doppler
            doppler_rate = std::clamp(doppler_rate, -5.0, 5.0); // clamp to a reasonable value
            doppler += doppler_rate / fs;                       // update doppler
            doppler = std::clamp(doppler,
                                 std::max(-range, -250.0),
                                 std::min(range, 250.0)); // clamp to range
        }

        sample_counter++;
    }
    catch (const std::exception &e)
    {
    }

    return response;
}

rapidjson::Value FalseTarget::to_json(rapidjson::Document::AllocatorType &allocator)
{

    rapidjson::Value target(rapidjson::kObjectType);

    target.AddMember("id", id, allocator);
    target.AddMember("type", rapidjson::Value(type.c_str(), allocator).Move(), allocator);

    try
    {
        if (type == TgtGen::VALID_TYPE[0])
        {
            target.AddMember("range", range, allocator);
            target.AddMember("delay", delay, allocator);
            target.AddMember("delay_samples", delay_samples, allocator);
            target.AddMember("doppler", doppler, allocator);
            target.AddMember("rcs", rcs, allocator);
        }
        else if (type == TgtGen::VALID_TYPE[1])
        {
            target.AddMember("range", range, allocator);
            target.AddMember("start_range", start_range, allocator);
            target.AddMember("delay", delay, allocator);
            target.AddMember("delay_samples", delay_samples, allocator);
            target.AddMember("doppler", doppler, allocator);
            target.AddMember("doppler_rate", doppler_rate, allocator);
            target.AddMember("rcs", rcs, allocator);
        }
        else
        {
            throw std::invalid_argument("Invalid target type");
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    return target;
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