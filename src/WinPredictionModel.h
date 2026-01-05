#pragma once

#include <iostream>
#include <cstdio>
#include <memory>
#include <string>
#include <array>

class WinPredictionModel {
public:
    WinPredictionModel();
    std::string infer(const std::string& match_json);
};