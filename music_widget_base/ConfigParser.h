#ifndef MUSIC_WIDGET_CONFIGPARSER_H
#define MUSIC_WIDGET_CONFIGPARSER_H

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string.h>
#include <sstream>
#include <unordered_map>
#include <algorithm>

#include "paop_config.h"
#include "Lexer.h"

#define TOKEN_INVALID -1
#define TOKEN_KEY 0
#define TOKEN_SPECIAL 1
#define TOKEN_COMMENT 2

#define CONF_DEFAULT_PLAYLIST_URL 1
#define CONF_PRELOAD_COUNT 2
#define CONF_SAVE_PREVIOUS 3

namespace config {
    class ConfigParser
    {
        public:
        ConfigParser();
        void openConfig();
        
        paop_config* parse();
        
        std::string getToken(const std::string &line, int &line_index, int &token_type, std::string &error_message);
        
        private:
        
        void setConfigProps(const std::string &key, std::string &value);
        
        std::unordered_map<std::string, int> m_config_map = {
                {"default_playlist_url", CONF_DEFAULT_PLAYLIST_URL},
                {"preload_count", CONF_PRELOAD_COUNT},
                {"save_previous", CONF_SAVE_PREVIOUS}
        };
        std::string m_config_path;
        std::ifstream m_stream;
        paop_config* pm_config;
        lexer m_lexer = {
                .special_tokens = {'=', ';'}
        };
    };
}

#endif //MUSIC_WIDGET_CONFIGPARSER_H