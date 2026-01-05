#pragma once

#include <iostream>
#include <dpp/dpp.h>
#include <fstream>
#include "MatchDB.h"
#include "RiotAPI.h"
#include "DataStructures.h"
#include "WinPredictionModel.h"

class LPBot
{
public:
    LPBot(const char* token, const char* broadcastChannel, int updateRate, const char* dir,
            const char* riotApiKey, const char* profileIconURL, const char* rankIconURL, const char* champIconURL);
    ~LPBot();

    void start();
    
    void loadPlayersToTrack(const std::string& path);
    void updateAllPlayerData();
    void fetchMatches(); // get match data for next match ID in queue

    // embeds
    dpp::embed playerInfoEmbed(const PlayerData& playerInfo);
    dpp::embed tierUpEmbed(const PlayerData& playerInfo);
    dpp::embed tierDownEmbed(const PlayerData& playerInfo);
    dpp::embed badGameEmbed(const Player& p, const std::string& champion, int kills, int deaths, int assists, double score);
    dpp::embed greatGameEmbed(const Player& p, const std::string& champion, int kills, int deaths, int assists, double score);

private:
    dpp::cluster bot;

    // loaded from env
    std::string broadcastChannel;
    int UPDATE_RATE;

    Players players;

    // private functions
    void updatePlayerData(const std::string& puuid);
    void playerDataChanged(const PlayerData& old, const PlayerData& current);
    void processNewMatch(const json& jsonData);

    std::string getPlayerNameFromPUUID(const std::string& puuid);
    std::string getPUUIDFromPlayerName(const std::string& playerName);

    // events
    void onLog(const dpp::log_t& event);
    void onReady(const dpp::ready_t& event);
    void onSlashCommand(const dpp::slashcommand_t& event);
    void incomingMessage(const dpp::message_create_t& event);

    // database
    MatchDB matchDB;
    // riot api
    RiotAPI riotAPI;
    // win prediction model
    WinPredictionModel model;


    // update thread
    std::thread update_thread;
    std::atomic<bool> stopUpdates {false};
    void update();


    // queue of matches to fetch
    std::unordered_set<std::string> matchIdsToFetch;
    std::mutex matchFetchMutex;

};