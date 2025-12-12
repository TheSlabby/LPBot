#include "LPBot.h"

LPBot::LPBot(const char* token, const char* dir)
    : bot(token, dpp::i_default_intents | dpp::i_message_content),
    matchDB(std::filesystem::path(dir) / "database.sqlite")
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

    // load players to track
    loadPlayersToTrack(std::filesystem::path(dir) / "players.txt");

    // start update thread
    update_thread = std::thread(&LPBot::update, this);
}

LPBot::~LPBot()
{
    stopUpdates = true; 

    if (update_thread.joinable())
        update_thread.join();
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

void LPBot::updatePlayerData(const std::string& puuid)
{
    auto playerData = riotAPI.getPlayerData(puuid);
    if (playerData)
    {
        matchDB.addPlayerData(*playerData);
    }
}
void LPBot::updateAllPlayerData()
{
    for (const auto& p : players)
    {
        updatePlayerData(p.puuid);
    }
}

void LPBot::loadPlayersToTrack(const std::string& path)
{
    std::vector<std::string> lines;
    std::ifstream file(path);
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            lines.push_back(line);
        }
        file.close();
    }

    for (const auto& playerString : lines)
    {
        // first check if we know their puuid
        if (auto foundPUUID = matchDB.getPuuid(playerString))
        {
            players.push_back({*foundPUUID, playerString});
        }
        else
        {
            std::cout << "couldnt find puuid" << std::endl;
            std::stringstream ss(playerString);
            // tokens
            std::string gameName;
            std::string tagLine;
            // load to tokens
            std::getline(ss, gameName, '#');
            std::getline(ss, tagLine, '#');

            auto puuid = riotAPI.getPUUID(gameName, tagLine);
            if (puuid) {
                matchDB.addPUUID(playerString, *puuid);
                players.push_back({*puuid, playerString});
            }
        }
    }

    for (const auto& p : players)
    {
        std::cout << "PUUID: " << p.puuid << ", player name: " << p.gameName << std::endl;
    }
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

void LPBot::update()
{
    while (!stopUpdates)
    {
        updateAllPlayerData();

        std::this_thread::sleep_for(std::chrono::seconds(30));
    }

    std::cout << "Update thread stopped" << std::endl;
}