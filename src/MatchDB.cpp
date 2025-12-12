#include "MatchDB.h"

MatchDB::MatchDB(const std::string& dbPath)
{
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc)
    {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Opened database: " << dbPath << " successfully!" << std::endl;
    }

    // create matches table
    const char* sql = 
            "CREATE TABLE IF NOT EXISTS matches ("
            "id TEXT PRIMARY KEY, "
            "match_json TEXT);";
    
    char* errMsg = 0;
    rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL ERROR: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}


MatchDB::~MatchDB()
{
    sqlite3_close(db);
}



bool MatchDB::saveMatch(const std::string& match_id, const std::string& json_data)
{
    sqlite3_stmt* stmt;
    std::string sql = "INSERT OR IGNORE INTO matches (id, match_json) VALUES (?,?)";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement" << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, match_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, json_data.c_str(), -1, SQLITE_STATIC);

    bool inserted = false;
    if (sqlite3_step(stmt) == SQLITE_DONE)
    {
        if (sqlite3_changes(db) > 0)
        {
            inserted = true;
        }
    }

    sqlite3_finalize(stmt);

    return inserted;
}
