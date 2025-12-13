#pragma once

#include <sqlite3.h>
#include <string>
#include <iostream>

class DBQuery
{
    sqlite3* db;
    sqlite3_stmt* stmt;

public:
    DBQuery(sqlite3* db, const std::string& sql) : db(db)  {
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
        }
    }

    ~DBQuery() {
        if (stmt) sqlite3_finalize(stmt);
    }

    void bind_arg(int i, int value) { sqlite3_bind_int(stmt, i, value); }
    void bind_arg(int i, int64_t value) { sqlite3_bind_int64(stmt, i, value); }
    void bind_arg(int i, const std::string& value) { sqlite3_bind_text(stmt, i, value.c_str(), -1, SQLITE_TRANSIENT); }

    template <typename T>
    void bind(int i, T value) { bind_arg(i, value); }

    void step() {
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw std::runtime_error(std::string("Exeuction failed: ") + sqlite3_errmsg(db));
        }
    }

    bool next() {
        return sqlite3_step(stmt) == SQLITE_ROW;
    }

    // get on current row
    std::string get_string(int col) {
        const char* txt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
        return txt ? std::string(txt) : std::string();
    }
    int get_int(int col) {
        return sqlite3_column_int(stmt, col);
    }
    int64_t get_int64(int col) {
        return sqlite3_column_int64(stmt, col);
    }
};