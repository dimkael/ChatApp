#pragma once

#include <string>
#include <algorithm>
#include <regex>
#include <unordered_map>

void toLower(std::string& str) {
	transform(str.begin(), str.end(), str.begin(), ::tolower);
}

class ChatBot {
public:
	ChatBot() {}
	~ChatBot() {}

	std::string ask(std::string question) {
		toLower(question);
		for (std::pair<std::string, std::string> entry : database) {
			std::regex exp = std::regex(".*" + entry.first + ".*");
			if (regex_match(question, exp)) {
				return entry.second;
			}
		}

		return "Sorry, i'm not so smart";
	}

private:
	std::unordered_map<std::string, std::string> database = {
		{ "hello", "Hi my human friend" },
		{ "how are you", "I'm fine, thanks" },
		{ "what is your name", "My name is ChatBot v.1"}
	};
};