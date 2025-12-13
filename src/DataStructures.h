#pragma once

#include <chrono>
#include <string>
#include <vector>

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

struct Player {
    std::string puuid;
    std::string gameName;
};
typedef std::vector<Player> Players;