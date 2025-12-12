#pragma once

#include <iostream>
#include <dpp/dpp.h>
#include "MatchDB.h"
#include "RiotAPI.h"

class LPBot
{
public:
    LPBot(const char* token);

    void start();

private:
    dpp::cluster bot;

    // events
    void onLog(const dpp::log_t& event);
    void onReady(const dpp::ready_t& event);
    void incomingMessage(const dpp::message_create_t& event);

    // database
    MatchDB matchDB;
    // riot api
    RiotAPI riotAPI;

};