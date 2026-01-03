#include <iostream>
#include "LPBot.h"

int main(int argc, char* arv[])
{
    // get lpbot env vars
    const char* token_env = std::getenv("DISCORD_TOKEN");
    const char* lpbot_dir = std::getenv("LPBOT_DIR");
    const char* riot_api_key = std::getenv("RIOT_API_KEY");
    const char* profile_icon_base_url = std::getenv("LPBOT_PROFILE_ICON_URL");
    const char* rank_icon_base_url = std::getenv("LPBOT_RANK_ICON_URL");
    const char* champion_icon_base_url = std::getenv("LPBOT_CHAMPION_ICON_URL");
    const char* broadcast_channel = std::getenv("LPBOT_BROADCAST_CHANNEL");
    const int update_rate = std::atoi(std::getenv("LPBOT_UPDATE_RATE"));

    LPBot bot(token_env, broadcast_channel, update_rate, lpbot_dir, riot_api_key, profile_icon_base_url, rank_icon_base_url, champion_icon_base_url);
    bot.start();
}