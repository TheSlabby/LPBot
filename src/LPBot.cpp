#include "LPBot.h"
#include <algorithm>

// util
static int tierToInt(const std::string& tier) {
    static constexpr std::array<std::pair<std::string_view, int>, 10> tiers {{
        {"IRON", 0},
        {"BRONZE", 1},
        {"SILVER", 2},
        {"GOLD", 3},
        {"PLATINUM", 4},
        {"EMERALD", 5},
        {"DIAMOND", 6},
        {"MASTER", 7},
        {"GRANDMASTER", 8},
        {"CHALLENGER", 9}
    }};
    for (const auto& t : tiers) {
        if (t.first == tier)
            return t.second;
    }
    return -1;
}
static int rankToInt(const std::string& rank) {
    static constexpr std::array<std::pair<std::string_view, int>, 4> ranks {{
        {"IV", 0},
        {"III", 1},
        {"II", 2},
        {"I", 3}
    }};
    for (const auto& r : ranks) {
        if (r.first == rank)
            return r.second;
    }
    return -1;
}

LPBot::LPBot(const char* token, const char* broadcastChannel, int updateRate, const char* dir,
             const char* riotApiKey, const char* profileIconURL, const char* rankIconURL) :
    bot(token, dpp::i_default_intents | dpp::i_message_content),
    matchDB(std::filesystem::path(dir) / "database.sqlite"),
    riotAPI(riotApiKey, profileIconURL, rankIconURL),
    broadcastChannel(broadcastChannel),
    UPDATE_RATE(updateRate)
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
    bot.on_slashcommand([this](const dpp::slashcommand_t& event){
        this->onSlashCommand(event);
    });

    std::cout << "Update rate (seconds): " << UPDATE_RATE << std::endl;

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
    // LEGACFY (we use slash command now)
    if (event.msg.content == "!lp")
    {
        std::string testpuuid = "BNS8uMQBu0w6ynoGYCu_9dIhE77bBiTLsgefPb4smHq1_-hNitFsQnmPKgu6Wguom9Sdov3toiBZKw";
        auto embed = playerInfoEmbed(matchDB.getLatestPlayerData(testpuuid).value());
        event.reply(embed);
    }
}
void LPBot::onReady(const dpp::ready_t& event)
{
    // create playerinfo slash command
    dpp::slashcommand lpCmd;
    lpCmd.set_name("lp")
        .set_description("View LP for player")
        .set_application_id(bot.me.id)
        .add_option(
            dpp::command_option(dpp::co_string, "name",
                    "Summoner Name & tagline (e.g. TheSlab#333)", true)
        );

    bot.global_command_create(lpCmd);

    // test
    // PlayerData current{"BNS8uMQBu0w6ynoGYCu_9dIhE77bBiTLsgefPb4smHq1_-hNitFsQnmPKgu6Wguom9Sdov3toiBZKw", "BRONZE", "IV", 0, 0, 0, std::chrono::system_clock::now()};
    // PlayerData old = current;
    // old.tier = "DIAMOND";
    // playerDataChanged(old, current);
}
void LPBot::onSlashCommand(const dpp::slashcommand_t& event)
{
    if (event.command.get_command_name() == "lp") {
        std::string playerName = std::get<std::string>(event.get_parameter("name"));
        auto puuid = getPUUIDFromPlayerName(playerName);
        if (puuid.empty()) {
            // failed to find player
            event.reply("I'm not tracking that player :(");
            return;
        }

        // search for player info
        auto playerData = matchDB.getLatestPlayerData(puuid);
        if (!playerData) {
            event.reply("I couldn't find their player data ;-;");
            return;
        }

        auto embed = playerInfoEmbed(*playerData);
        event.reply(embed);
    }
}

void LPBot::updatePlayerData(const std::string& puuid)
{
    // get previous data
    auto prevPlayerData = matchDB.getLatestPlayerData(puuid);

    auto playerData = riotAPI.getPlayerData(puuid);
    if (playerData)
    {
        const auto& currentData = *playerData;
        matchDB.addPlayerData(currentData);

        // check lp change
        if (prevPlayerData) {
            const auto& previousData = *prevPlayerData;
            if (   (previousData.lp != currentData.lp)
                || (previousData.rank != currentData.rank)
                || (previousData.tier != currentData.tier)        
            ) {
                playerDataChanged(previousData, currentData);
            }
        }
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

        std::this_thread::sleep_for(std::chrono::seconds(UPDATE_RATE));
    }

    std::cout << "Update thread stopped" << std::endl;
}

std::string LPBot::getPlayerNameFromPUUID(const std::string& puuid)
{
    for (const auto& p : players) {
        if (p.puuid == puuid)
            return p.gameName;
    }

    return "N/A";
}
std::string LPBot::getPUUIDFromPlayerName(const std::string& playerName)
{
    for (const auto& p : players) {
    if (p.gameName == playerName)
        return p.puuid;
    }

    return std::string();
}

void LPBot::playerDataChanged(const PlayerData& old, const PlayerData& current)
{
    auto puuid = current.puuid;
    auto playerName = getPlayerNameFromPUUID(puuid);

    // check for rank change
    int currentTier = tierToInt(current.tier);
    int oldTier = tierToInt(old.tier);
    if (currentTier > oldTier) {
        // RANKUP
        auto embed = tierUpEmbed(current);
        auto msg = dpp::message(embed);
        msg.channel_id = broadcastChannel;
        bot.message_create(msg);
    } else if (currentTier < oldTier) {
        // DERANK
        auto embed = tierDownEmbed(current);
        auto msg = dpp::message(embed);
        msg.channel_id = broadcastChannel;
        bot.message_create(msg);
    }
}

dpp::embed LPBot::playerInfoEmbed(const PlayerData& playerInfo)
{
    std::string playerName = getPlayerNameFromPUUID(playerInfo.puuid);
    std::string rankString = playerInfo.tier + " " + playerInfo.rank;
    std::string winLossString = std::to_string(playerInfo.wins) + "/" + std::to_string(playerInfo.losses);

    // get images
    auto summonerInfo = riotAPI.getSummonerInfo(playerInfo.puuid);
    int iconId = summonerInfo ? summonerInfo.value().iconID : 1;
    std::string icon = riotAPI.getIconFromID(iconId);
    std::string rankImage = riotAPI.getRankImage(playerInfo);
    std::cout << "rank image: " << rankImage << std::endl;

    // create author embed
    dpp::embed_author authorEmbed = dpp::embed_author();
    authorEmbed.name = playerName;
    authorEmbed.icon_url = icon;
    std::string urlName = playerName;
    std::replace(urlName.begin(), urlName.end(), '#', '-');
    authorEmbed.url = "https://www.op.gg/summoners/na/" + urlName;

    return dpp::embed()
        .set_color(dpp::colors::red_blood)
        .set_title("Solo/Duo Ranked Info")
        .set_description(std::string("**Current Rank:** ") + rankString)
        .add_field(
            "LP",
            std::to_string(playerInfo.lp) + " LP",
            true
        )
        .add_field(
            "W/L Record",
            winLossString,
            true
        )
        .set_author(authorEmbed)
        .set_image(rankImage);
}

dpp::embed LPBot::tierUpEmbed(const PlayerData& playerInfo)
{
    std::string playerName = getPlayerNameFromPUUID(playerInfo.puuid);

    // get images
    auto summonerInfo = riotAPI.getSummonerInfo(playerInfo.puuid);
    int iconId = summonerInfo ? summonerInfo.value().iconID : 1;
    std::string icon = riotAPI.getIconFromID(iconId);
    std::string rankImage = riotAPI.getRankImage(playerInfo);

    // create author embed
    dpp::embed_author authorEmbed = dpp::embed_author();
    authorEmbed.name = playerName;
    authorEmbed.icon_url = icon;
    std::string urlName = playerName;
    std::replace(urlName.begin(), urlName.end(), '#', '-');
    authorEmbed.url = "https://www.op.gg/summoners/na/" + urlName;

    return dpp::embed()
        .set_color(dpp::colors::green_snake)
        .set_title(std::string("Ranked up to ") + playerInfo.tier + std::string("!"))
        .set_author(authorEmbed)
        .set_image(rankImage);
}

dpp::embed LPBot::tierDownEmbed(const PlayerData& playerInfo)
{
    std::string playerName = getPlayerNameFromPUUID(playerInfo.puuid);

    // get images
    auto summonerInfo = riotAPI.getSummonerInfo(playerInfo.puuid);
    int iconId = summonerInfo ? summonerInfo.value().iconID : 1;
    std::string icon = riotAPI.getIconFromID(iconId);
    std::string rankImage = riotAPI.getRankImage(playerInfo);

    // create author embed
    dpp::embed_author authorEmbed = dpp::embed_author();
    authorEmbed.name = playerName;
    authorEmbed.icon_url = icon;
    std::string urlName = playerName;
    std::replace(urlName.begin(), urlName.end(), '#', '-');
    authorEmbed.url = "https://www.op.gg/summoners/na/" + urlName;

    return dpp::embed()
    .set_color(dpp::colors::green_snake)
    .set_title(playerName + std::string(" is a pathetic LOSER"))
    .set_description(
        std::string("they just deranked to ") +
        playerInfo.tier +
        std::string("! LOL!! fucking die already, trash <3")
    )
    .set_author(authorEmbed)
    .set_image(rankImage);

}
