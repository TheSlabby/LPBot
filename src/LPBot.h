#pragma once

#include <iostream>
#include <dpp/dpp.h>
#include <fstream>
#include "MatchDB.h"
#include "RiotAPI.h"

class LPBot
{
public:
    LPBot(const char* token, const char* dir);
    ~LPBot();

    void start();
    
    void loadPlayersToTrack(const std::string& path);
    void updateAllPlayerData();

private:
    dpp::cluster bot;

    Players players;

    // private functions
    void updatePlayerData(const std::string& puuid);
    void playerDataChanged(const PlayerData& old, const PlayerData& current);

    std::string getPlayerNameFromPUUID(const std::string& puuid);

    // events
    void onLog(const dpp::log_t& event);
    void onReady(const dpp::ready_t& event);
    void incomingMessage(const dpp::message_create_t& event);

    // database
    MatchDB matchDB;
    // riot api
    RiotAPI riotAPI;


    // update thread
    std::thread update_thread;
    std::atomic<bool> stopUpdates {false};
    void update();

};