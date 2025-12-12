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

    runQuery( "CREATE TABLE IF NOT EXISTS matches ("
            "id TEXT PRIMARY KEY, "
            "match_json TEXT);");
    // create puuid table
    runQuery("CREATE TABLE IF NOT EXISTS puuids ("
            "player_name TEXT PRIMARY KEY, "
            "puuid TEXT);");
    
    // create playerData table
    runQuery("CREATE TABLE IF NOT EXISTS player_data ("
            "timestamp INTEGER NOT NULL PRIMARY KEY,"
            "puuid TEXT NOT NULL,"
            "rank TEXT,"
            "tier TEXT,"
            "lp INTEGER,"
            "wins INTEGER,"
            "losses INTEGER"
            ");"
    );
}


MatchDB::~MatchDB()
{
    sqlite3_close(db);
}


void MatchDB::runQuery(const std::string& query)
{
    const char* sql = query.c_str();
    
    char* errMsg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL ERROR: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

void MatchDB::addPlayerData(const PlayerData& playerData)
{
    sqlite3_stmt* stmt;
    std::string sql = "INSERT INTO player_data"
                    "(puuid, timestamp, rank, tier, lp, wins, losses)"
                    "VALUES (?,?,?,?,?,?,?)";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement" << std::endl;
        return;
    }

    auto epochTime = playerData.timestamp.time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(epochTime);
    sqlite3_int64 timestamp_ms = ms.count();

    sqlite3_bind_text(stmt, 1, playerData.puuid.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, timestamp_ms);
    sqlite3_bind_text(stmt, 3, playerData.rank.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, playerData.tier.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, playerData.lp);
    sqlite3_bind_int(stmt, 6, playerData.wins);
    sqlite3_bind_int(stmt, 7, playerData.losses);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Execution error when inserting data: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
}

void MatchDB::addPUUID(const std::string& playerName, const std::string& puuid)
{
    sqlite3_stmt* stmt;
    std::string sql = "INSERT INTO puuids (player_name, puuid) VALUES (?,?)";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement" << std::endl;
        return;
    }
    sqlite3_bind_text(stmt, 1, playerName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, puuid.c_str(), -1, SQLITE_STATIC);

    // run query
    sqlite3_step(stmt);
    // clean up
    sqlite3_finalize(stmt);

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

std::optional<std::string> MatchDB::getPuuid(const std::string& playerName)
{
    std::optional<std::string> puuid = std::nullopt;
    sqlite3_stmt* stmt;
    std::string sql = "SELECT * FROM puuids WHERE player_name = ?";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement" << std::endl;
    }

    sqlite3_bind_text(stmt, 1, playerName.c_str(), -1, SQLITE_STATIC);

    int result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) {
        const unsigned char* columnData = sqlite3_column_text(stmt, 1);

        if (columnData) {
            puuid = reinterpret_cast<const char*>(columnData);
        }
    }

    sqlite3_finalize(stmt);
    return puuid;
}