#include "RiotAPI.h"

RiotAPI::RiotAPI(const char* apiKey, const char* profileIconURL, const char* rankIconURL) :
    API_KEY(apiKey),
    PROFILE_ICON_BASE_URL(profileIconURL),
    RANK_ICON_BASE_URL(rankIconURL)

{
    // ctor
}


std::optional<std::string> RiotAPI::getPUUID(const std::string& gameName, const std::string& tagLine)
{
    httplib::Client client(URL_BASE_AMERICAS);

    std::string path = "/riot/account/v1/accounts/by-riot-id/"
            + gameName + "/"
            + tagLine
            + "?api_key=" + API_KEY;
        
    if (auto res = client.Get(path)) {
        if (res->status == 200) {
            try {
                json j = json::parse(res->body);
                return j["puuid"];
            } catch (json::parse_error& e) {
                std::cerr << "JSON error: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "ERORR: status != 200" << std::endl;
        }
    } else {
        std::cerr << "Network error" << std::endl;
    }
    
    return std::nullopt;
}

std::optional<PlayerData> RiotAPI::getPlayerData(const std::string& puuid)
{
    httplib::Client client(URL_BASE_NA1);

    std::string path = "/lol/league/v4/entries/by-puuid/"
            + puuid
            + "?api_key=" + API_KEY;
        
    if (auto res = client.Get(path)) {
        if (res->status == 200) {
            try {
                json j = json::parse(res->body);

                // grab ranked info
                for (const auto& obj : j)
                {
                    std::string queueType = obj["queueType"];
                    if (queueType == "RANKED_SOLO_5x5") {
                        // fill player data
                        PlayerData data;
                        data.puuid = puuid;
                        data.timestamp = std::chrono::system_clock::now();
                        data.rank = obj["rank"];
                        data.tier = obj["tier"];
                        data.lp = obj["leaguePoints"];
                        data.wins = obj["wins"];
                        data.losses = obj["losses"];

                        return data;
                    }
                }

            } catch (json::parse_error& e) {
                std::cerr << "JSON error: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "ERORR: status != 200" << std::endl;
        }
    } else {
        std::cerr << "Network error" << std::endl;
    }
    
    return std::nullopt;

}

std::optional<SummonerInfo> RiotAPI::getSummonerInfo(const std::string& puuid)
{
    httplib::Client client(URL_BASE_NA1);

    std::string path = "/lol/summoner/v4/summoners/by-puuid/"
            + puuid
            + "?api_key=" + API_KEY;
        
    if (auto res = client.Get(path)) {
        if (res->status == 200) {
            try {
                json j = json::parse(res->body);

                SummonerInfo summonerInfo;
                summonerInfo.iconID = j["profileIconId"];
                summonerInfo.level = j["summonerLevel"];
                return summonerInfo;

            } catch (json::parse_error& e) {
                std::cerr << "JSON error: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "ERORR: status != 200" << std::endl;
        }
    } else {
        std::cerr << "Network error" << std::endl;
    }
    
    return std::nullopt;
}

std::string RiotAPI::getIconFromID(int id)
{
    return PROFILE_ICON_BASE_URL + std::to_string(id) + ".png";
}

std::string RiotAPI::getRankImage(const PlayerData& playerData)
{
    std::string tier = playerData.tier;
    std::transform(tier.begin(), tier.end(), tier.begin(),
                [](unsigned char c) { return std::tolower(c); }
    );
    return RANK_ICON_BASE_URL + tier + ".png";
}