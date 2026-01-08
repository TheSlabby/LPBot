#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <array>

// rank util functions
inline int tierToInt(const std::string& tier) {
    static constexpr std::array<std::pair<std::string_view, int>, 10> tiers {{
        {"IRON", 0},
        {"BRONZE", 1},
        {"SILVER", 2},
        {"GOLD", 3},
        {"PLATINUM", 4},
        {"EMERALD", 5},
        {"DIAMOND", 6},
        {"MASTER", 7},
        {"GRANDMASTER", 8},
        {"CHALLENGER", 9}
    }};
    for (const auto& t : tiers) {
        if (t.first == tier)
            return t.second;
    }
    return -1;
}
inline int rankToInt(const std::string& rank) {
    static constexpr std::array<std::pair<std::string_view, int>, 4> ranks {{
        {"IV", 0},
        {"III", 1},
        {"II", 2},
        {"I", 3}
    }};
    for (const auto& r : ranks) {
        if (r.first == rank)
            return r.second;
    }
    return -1;
}



// player data struct
struct PlayerData {
    std::string puuid;
    
    // solo/duo
    std::string tier;
    std::string rank;
    int lp;
    int wins;
    int losses;
    std::chrono::system_clock::time_point timestamp;

};

// from Summoner-v4
struct SummonerInfo {
    int iconID;
    long level;
};

struct Player {
    std::string puuid;
    std::string gameName;
};
typedef std::vector<Player> Players;