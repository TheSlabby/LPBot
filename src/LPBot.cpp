#include "LPBot.h"

LPBot::LPBot(const char* token)
    : bot(token, dpp::i_default_intents | dpp::i_message_content),
    matchDB("/home/walker/LPBot/database.sqlite")
{
    // events
    bot.on_message_create([this](const dpp::message_create_t& event){
        this->incomingMessage(event);
    });
    bot.on_ready([this](const dpp::ready_t& event){
        this->onReady(event);
    });
    bot.on_log([this](const dpp::log_t& event){
        this->onLog(event);
    });

    // grab puuid
    auto puuid = riotAPI.getPUUID("TheSlab", "333");
    if (puuid) {
        std::cout << " got puuid: " << *puuid << std::endl;
        // get ranked info
        riotAPI.getPlayerData(*puuid);
    }

    bool inserted = matchDB.saveMatch("test", "testData");
    std::cout << "insreted match: " << inserted << std::endl;
}

void LPBot::start()
{
    bot.start(dpp::st_wait);
}

// events
void LPBot::incomingMessage(const dpp::message_create_t& event)
{
    if (event.msg.content == "ping")
    {
        event.reply("pong");
    }
}
void LPBot::onReady(const dpp::ready_t& event)
{
    // bot is ready
}



void LPBot::onLog(const dpp::log_t& event)
{
    if (event.severity < dpp::ll_info)
        return;
    
    std::string prefix;
    switch(event.severity)
    {
        case dpp::ll_info: prefix = "[INFO]"; break;
        case dpp::ll_warning: prefix = "[WARNING]"; break;
        case dpp::ll_error: prefix = "[ERROR]"; break;
        case dpp::ll_critical: prefix = "[CRITICAL]"; break;
        default: prefix = "[LOG]"; break;
    }

    std::cout << prefix << " " << event.message << std::endl;
}