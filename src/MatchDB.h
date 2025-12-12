#pragma once

#include <sqlite3.h>
#include <string>
#include <iostream>
#include <optional>
#include "DataStructures.h"

class MatchDB
{
public:
    MatchDB(const std::string& dbPath);
    ~MatchDB();

    void runQuery(const std::string& query);
    bool saveMatch(const std::string& match_id, const std::string& json_data);
    
    std::optional<std::string> getPuuid(const std::string& playerName);
    void addPUUID(const std::string& playerName, const std::string& puuid);

    void addPlayerData(const PlayerData& playerData);

private:
    sqlite3* db;
};