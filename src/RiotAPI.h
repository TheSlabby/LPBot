#pragma once

#include <iostream>
#include <nlohmann/json.hpp>
#include <httplib.h>
#include <optional>
#include <chrono>
#include "DataStructures.h"
#include <cctype>
#include <algorithm>


using json = nlohmann::json;

class RiotAPI
{
public:
    RiotAPI();

    std::optional<std::string> getPUUID(const std::string& gameName, const std::string& tagLine);

    std::optional<PlayerData> getPlayerData(const std::string& puuid);

    std::optional<SummonerInfo> getSummonerInfo(const std::string& puuid);

    // imgs
    std::string getIconFromID(int id);
    std::string getRankImage(const PlayerData& playerData);

    // constants
    inline static const std::string URL_BASE_AMERICAS = "https://americas.api.riotgames.com";
    inline static const std::string URL_BASE_NA1 = "https://na1.api.riotgames.com";

    inline static std::string API_KEY;

private:
    std::string iconBaseURL;
    std::string rankImageBaseURL;

};