#include "RiotAPI.h"

RiotAPI::RiotAPI()
{
    API_KEY = std::getenv("RIOT_API_KEY");

    // url base

}


std::optional<std::string> RiotAPI::getPUUID(const std::string& gameName, const std::string& tagLine)
{
    httplib::Client client(URL_BASE_AMERICAS);

    std::string path = "/riot/account/v1/accounts/by-riot-id/"
            + gameName + "/"
            + tagLine
            + "?api_key=" + API_KEY;
    
    std::cout << "making request to: " << URL_BASE_AMERICAS << path << std::endl;
    
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
    
    std::cout << "making request to: " << URL_BASE_NA1 << path << std::endl;
    
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