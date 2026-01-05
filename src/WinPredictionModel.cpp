#include "WinPredictionModel.h"

WinPredictionModel::WinPredictionModel()
{
    // ctor
}

// todo keep program open & just add to stdin & read stdout 
std::string WinPredictionModel::infer(const std::string& match_json)
{
    std::array<char, 1024> buffer;
    std::string result;
    
    std::string cmd = "cd ~/hextrack-ai && echo '" + match_json + "' | ./.venv/bin/python3 inference.py";
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd.c_str(), "r"), pclose);

    if (!pipe) {
        throw std::runtime_error("popen() failed");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result;
}