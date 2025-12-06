#include <iostream>
#include "LPBot.h"

int main(int argc, char* arv[])
{
    const char* token_env = std::getenv("DISCORD_TOKEN");
    LPBot bot(token_env);
    bot.start();
    // std::cout << "using discord token: " << token_env << std::endl;

    // dpp::cluster bot(token_env, dpp::i_default_intents | dpp::i_message_content);
    
    // bot.on_log(dpp::utility::cout_logger());

    // bot.on_message_create([](const dpp::message_create_t& event) {
    //     if (event.msg.content == "!ping")
    //     {
    //         event.reply("Pong", true);
    //     }
    // });
}