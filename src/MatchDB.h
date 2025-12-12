#pragma once

#include <sqlite3.h>
#include <string>
#include <iostream>

class MatchDB
{
public:
    MatchDB(const std::string& dbPath);
    ~MatchDB();

    bool saveMatch(const std::string& match_id, const std::string& json_data);

private:
    sqlite3* db;
};