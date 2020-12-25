#include <iostream>
#include <thread>
#include <algorithm>
#include <atomic>
#include <uwebsockets/App.h>
#include "ChatBot.h"

struct UserConnection {
    unsigned long user_id;
    std::string user_name;
};

int main() {
    std::atomic_ulong latest_user_id = 10;

    std::vector<std::thread*> threads(std::thread::hardware_concurrency());

    // Bot has 9 id
    ChatBot bot;

    transform(threads.begin(), threads.end(), threads.begin(), [&latest_user_id, &bot](std::thread* thr) {

        return new std::thread([&latest_user_id, &bot]() {

            uWS::App().ws<UserConnection>("/*", {

                .open = [&latest_user_id, &bot](auto* ws) {
                    UserConnection* data = (UserConnection*)ws->getUserData();

                    data->user_id = latest_user_id++;
                    data->user_name = "USER";

                    ws->subscribe("broadcast");
                    ws->subscribe("user#" + std::to_string(data->user_id));

                    std::cout << "[SERVER]: " + data->user_name << "[" << data->user_id << "] connected to the chat" << '\n';
                    std::cout << "Total users connected: " << data->user_id - 9 << '\n';
                },

                .message = [&](auto* ws, std::string_view message, uWS::OpCode opCode) {
                    UserConnection* data = (UserConnection*)ws->getUserData();
                    std::string_view text;
                    std::string msg;
                    
                    if (message.substr(0, 9) == "/setname ") {
                        // User set up his name
                        std::string_view name = message.substr(9);

                        if (name.find(",") == std::string::npos && name.length() < 256) {
                            std::cout << data->user_name << "[" << data->user_id << "] set his name as \"" << name << "\"\n";

                            std::string bc_msg = "[GLOBAL] " + data->user_name + "[" + std::to_string(data->user_id) + "] set his name as \"" + std::string(name) + "\"\n";
                            ws->publish("broadcast", bc_msg, opCode, false);

                            data->user_name = name;
                        }
                        else {
                            std::string user_msg = "Setting name failed. Name shouldn't contain \",\" symbols and its length should be less than 256 letters\n";
                            ws->publish("user#" + std::to_string(data->user_id), user_msg, opCode, false);
                        }
                    }
                    else if (message.substr(0, 4) == "/pm ") {
                        // User sent private message
                        text = message.substr(4);

                        int pos = text.find(" ");
                        if (pos != std::string::npos) {
                            std::string id_str = std::string(text.substr(0, pos));
                            std::string message_str = std::string(text.substr(pos + 1));
                            unsigned long id = std::stoul(id_str);

                            if (id <= latest_user_id) {
                                msg = "[PRIVATE] " + data->user_name + "[" + std::to_string(data->user_id) + "]: " + message_str + "\n";
                                ws->publish("user#" + id_str, msg, opCode, false);
                            }
                            else {
                                msg = "User with id " + id_str + " doesn't exist\n";
                                ws->publish("user#" + std::to_string(data->user_id), msg, opCode, false);
                            }
                        }
                    }
                    else if (message.substr(0, 5) == "/bot ") {
                        // User sent message to bot
                        text = message.substr(4);
                        msg = "[PRIVATE] BOT: " + bot.ask(std::string(text)) + "\n";
                        ws->publish("user#" + std::to_string(data->user_id), msg, opCode, false);
                    }
                    else {
                        // User sent public message
                        msg = "[GLOBAL] " + data->user_name + "[" + std::to_string(data->user_id) + "]: " + std::string(message) + "\n";
                        ws->publish("broadcast", msg, opCode, false);
                    }
                },

                .close = [](auto* ws, int code, std::string_view message) {
                    UserConnection* data = (UserConnection*)ws->getUserData();
                    std::cout << data->user_name << "[" << std::to_string(data->user_id) << "] disconnected from the chat\n";
                }
            }).listen(9999, [](auto* token) { 
                if (token) std::cout << "Server started and listening on port 9999\n";
                else std::cout << "Server failed to start\n";
            }).run();
        });
    });

    for_each(threads.begin(), threads.end(), [](std::thread* thr) { thr->join(); });
}
