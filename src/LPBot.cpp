#include "LPBot.h"
#include <algorithm>
#include <ctime>

LPBot::LPBot(const char* token, const char* broadcastChannel, int updateRate, const char* dir,
             const char* riotApiKey, const char* profileIconURL, const char* rankIconURL, const char* champIconURL) :
    bot(token, dpp::i_default_intents | dpp::i_message_content),
    matchDB(std::filesystem::path(dir) / "database.sqlite"),
    riotAPI(riotApiKey, profileIconURL, rankIconURL, champIconURL),
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

    // get recent match IDs
    auto matchHistory = riotAPI.getRecentMatches(puuid);
    if (matchHistory)
    {
        std::lock_guard<std::mutex> lock(matchFetchMutex); // get match set mutex
        std::vector<std::string> matches = matchHistory.value();
        for (const std::string& match : matches)
        {
            // only add matches to queue if they're not already in db
            if (!matchDB.getMatch(match))
                matchIdsToFetch.insert(match);
        }
    }
    else
    {
        std::cout << "couldn't get match history for: " << puuid << std::endl;
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
        fetchMatches();
        tryDailyTrigger();

        std::this_thread::sleep_for(std::chrono::seconds(UPDATE_RATE));
    }

    std::cout << "Update thread stopped" << std::endl;
}

void LPBot::fetchMatches()
{
    std::string matchId;
    {
        std::lock_guard<std::mutex> lock(matchFetchMutex);
        if (matchIdsToFetch.empty()) return;
        auto it = matchIdsToFetch.begin();
        matchId = *it;
        matchIdsToFetch.erase(it);

        std::cout << "fetching match json for id: " << matchId << ", queue size: " << matchIdsToFetch.size() << std::endl;
    }

    auto j = riotAPI.getMatch(matchId);
    if (j)
    {
        json jsonData = *j;
        std::string matchString = jsonData.dump();
        matchDB.addMatch(matchId, matchString);
        processNewMatch(jsonData);
    }
    else
    {
        std::cout << "failed to get match: " << matchId << std::endl;
    }
}

void LPBot::processNewMatch(const json& jsonData)
{
    // run match through model
    std::unordered_map<std::string, double> scores;
    try {
        std::string raw_json = model.infer(jsonData.dump());
        json data = json::parse(raw_json);
        for (const auto& player : players) {
            // get name, not tag
            std::string playerName = player.gameName;
            size_t pos = playerName.find('#');
            std::string key = playerName.substr(0, pos);
            if (data.contains(key)) {
                scores[playerName] = data[key];
            }
        }
    } catch (json::parse_error& e) {
        std::cout << "json parse failed" << std::endl;
    }


    const auto& participants = jsonData["info"]["participants"];
    for (const auto& p : participants)
    {
        std::string puuid = p["puuid"];
        auto it = std::find_if(players.begin(), players.end(), [&](const Player& player){
            return player.puuid == puuid;
        });
        if (it != players.end())
        {
            Player player = *it;
            // we are tracking this player
            int kills = p["kills"];
            int deaths = p["deaths"];
            int assists = p["assists"];
            int deathsSafe = (deaths > 0) ? deaths : 1; // avoid division by 0
            std::string champion = p["championName"];
            float kda = static_cast<float>(kills + assists) / deathsSafe;
            if (kda > 6.0f)
            {
                auto embed = greatGameEmbed(player, champion, kills, deaths, assists, scores[player.gameName]);
                auto msg = dpp::message(embed);
                msg.channel_id = broadcastChannel;
                bot.message_create(msg);
            }
            else if (kda < 1.0f && deaths > 0 && BAD_EMBED_ENABLED) // deaths > 0 to make sure no remake
            {
                auto embed = badGameEmbed(player, champion, kills, deaths, assists, scores[player.gameName]);
                auto msg = dpp::message(embed);
                msg.channel_id = broadcastChannel;
                bot.message_create(msg);
            }
        }
    }
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

void LPBot::tryDailyTrigger()
{
    auto now = std::chrono::system_clock::now();
    auto epochTime = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&epochTime);

    if (localTime->tm_hour >= DAILY_TRIGGER_TIME && lastRunDay != localTime->tm_yday)
    {
        lastRunDay = localTime->tm_yday;
        dailyTrigger();
    }
}
void LPBot::dailyTrigger() // runs once a day
{
    // get current time in ms
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::system_clock::now().time_since_epoch()).count();
    // int64_t yesterday = now - (86400000);
    
    std::vector<int> lpDif;
    for (const auto& player : players) {
        lpDif.push_back(matchDB.getLPDiff(player.puuid, SEASON_START_TIMESTAMP, now));
        // lpDif[player.gameName]  = matchDB.getLPDiff(player.puuid, yesterday, now);
        // TODO maybe get avg AI score of the day
    }
    
    auto embed = dailyEmbed(lpDif);
    auto msg = dpp::message(embed);
    msg.channel_id = broadcastChannel;
    bot.message_create(msg);
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
        .set_title(playerName + std::string(" is a LOSER!"))
        .set_description(std::string("They just deranked to ") + playerInfo.tier + std::string("! LOL"))
        .set_author(authorEmbed)
        .set_image(rankImage);
}

dpp::embed LPBot::badGameEmbed(const Player& player, const std::string& champion, int kills, int deaths, int assists, double score)
{
    std::string playerName = player.gameName;
    std::string puuid = player.puuid;

    // get images
    auto summonerInfo = riotAPI.getSummonerInfo(puuid);
    int iconId = summonerInfo ? summonerInfo.value().iconID : 1;
    std::string icon = riotAPI.getIconFromID(iconId);
    std::string champIcon = riotAPI.getChampionIcon(champion);

    // create author embed
    dpp::embed_author authorEmbed = dpp::embed_author();
    authorEmbed.name = playerName;
    authorEmbed.icon_url = icon;
    std::string urlName = playerName;
    std::replace(urlName.begin(), urlName.end(), '#', '-');
    authorEmbed.url = "https://www.op.gg/summoners/na/" + urlName;

    std::string ansiKDA = "```ansi\n";
    ansiKDA += "\u001b[0;37m" + std::to_string(kills) + " / "; // White Kills
    ansiKDA += "\u001b[0;31m" + std::to_string(deaths);        // RED Deaths
    ansiKDA += "\u001b[0;37m / " + std::to_string(assists);    // White Assists
    ansiKDA += "\n```";
    
    // ai score
    int rounded_score = static_cast<int>(std::round(score * 100));
    std::string footerText = "AI Score: " + std::to_string(rounded_score) + "%";
    
    return dpp::embed()
        .set_color(dpp::colors::red_blood)
        .set_title("What a TERRIBLE game! LOL")
        .set_author(authorEmbed)
        .set_thumbnail(champIcon)
        .set_description(ansiKDA)
        .set_footer(dpp::embed_footer().set_text(footerText));
}

dpp::embed LPBot::greatGameEmbed(const Player& player, const std::string& champion, int kills, int deaths, int assists, double score)
{
    std::string playerName = player.gameName;
    std::string puuid = player.puuid;

    // get images
    auto summonerInfo = riotAPI.getSummonerInfo(puuid);
    int iconId = summonerInfo ? summonerInfo.value().iconID : 1;
    std::string icon = riotAPI.getIconFromID(iconId);
    std::string champIcon = riotAPI.getChampionIcon(champion);

    // create author embed
    dpp::embed_author authorEmbed = dpp::embed_author();
    authorEmbed.name = playerName;
    authorEmbed.icon_url = icon;
    std::string urlName = playerName;
    std::replace(urlName.begin(), urlName.end(), '#', '-');
    authorEmbed.url = "https://www.op.gg/summoners/na/" + urlName;

    std::string ansiKDA = "```ansi\n";
    ansiKDA += "\u001b[1;32m" + std::to_string(kills);         // Bold Green Kills!
    ansiKDA += "\u001b[0;37m / " + std::to_string(deaths);     // White Deaths
    ansiKDA += "\u001b[0;37m / " + std::to_string(assists);    // White Assists
    ansiKDA += "\n```";

    // ai score
    int rounded_score = static_cast<int>(std::round(score * 100));
    std::string footerText = "AI Score: " + std::to_string(rounded_score) + "%";

    return dpp::embed()
        .set_color(dpp::colors::red_blood)
        .set_title("An AMAZING game! :O")
        .set_author(authorEmbed)
        .set_thumbnail(champIcon)
        .set_description(ansiKDA)
        .set_footer(dpp::embed_footer().set_text(footerText));
}

dpp::embed LPBot::dailyEmbed(const std::vector<int>& lpDiff)
{
    struct Entry {
        std::string name;
        int diff;
    };
    std::vector<Entry> leaderboard;

    for (size_t i = 0; i < players.size(); ++i) {
        if (lpDiff[i] == 0) continue; 

        leaderboard.push_back({players[i].gameName, lpDiff[i]});
    }

    std::sort(leaderboard.begin(), leaderboard.end(), [](const Entry& a, const Entry& b) {
        return a.diff > b.diff;
    });

    std::string description = "```ansi\n";
    
    if (leaderboard.empty()) {
        description = "No ranked games played yet this season!";
    } else {
        for (size_t i = 0; i < leaderboard.size(); ++i) {
            int diff = leaderboard[i].diff;
            std::string name = leaderboard[i].name;
            std::string prefix = "";
            if (i == 0) prefix = "🥇 ";
            else if (i == 1) prefix = "🥈 ";
            else if (i == 2) prefix = "🥉 ";
            else prefix = std::to_string(i + 1) + ". ";
            std::string color = (diff >= 0) ? "\u001b[1;32m+" : "\u001b[1;31m";
            description += prefix + color + std::to_string(diff) + " LP";
            description += "\u001b[0;37m   " + name + "\n";
        }
        description += "```";
    }

    return dpp::embed()
        .set_title("🏆 Season 2026 Leaderboard")
        .set_color(dpp::colors::gold)
        .set_description(description)
        .set_timestamp(time(0));
}