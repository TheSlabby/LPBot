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
    RiotAPI(const char* apiKey, const char* profileIconURL, const char* rankIconURL, const char* champIconURL);

    std::optional<std::string> getPUUID(const std::string& gameName, const std::string& tagLine);

    std::optional<PlayerData> getPlayerData(const std::string& puuid);

    std::optional<SummonerInfo> getSummonerInfo(const std::string& puuid);

    std::optional<json> getMatch(const std::string& matchId);

    std::optional<std::vector<std::string>> getRecentMatches(const std::string& puuid);

    // imgs
    std::string getIconFromID(int id);
    std::string getRankImage(const PlayerData& playerData);
    std::string getChampionIcon(const std::string& championName);

    // constants
    inline static const std::string URL_BASE_AMERICAS = "https://americas.api.riotgames.com";
    inline static const std::string URL_BASE_NA1 = "https://na1.api.riotgames.com";


private:
    std::string API_KEY;
    std::string PROFILE_ICON_BASE_URL;
    std::string RANK_ICON_BASE_URL;
    std::string CHAMPION_ICON_BASE_URL;

};