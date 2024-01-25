#ifndef MUSIC_WIDGET_CONFIGPARSER_H
#define MUSIC_WIDGET_CONFIGPARSER_H

#include <cstdio>
#include <iostream>
#include <fstream>
#include <map>

#include "paop_config.h"
#include "Lexer.h"

#define CONFIG_PATH "~

namespace config {
    class ConfigParser
    {
        public:
        ConfigParser();
        void openConfig();
        
        void parse();
        
        std::string getToken(const std::string &line);
        
        private:
        std::string m_config_path;
        std::ifstream m_stream;
        const paop_config* pm_config;
        lexer m_lexer = {
                .special_tokens = {'=', ';'}
        };
//         m_available_options;
    };
}

#endif //MUSIC_WIDGET_CONFIGPARSER_H