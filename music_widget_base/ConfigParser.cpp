#include "ConfigParser.h"

namespace config {
    ConfigParser::ConfigParser()
    {
        pm_config = (const paop_config*)malloc(sizeof(paop_config));
        m_config_path = std::string(getenv("HOME")) + "/config/paoplayer/config";
        openConfig();
    }
    
    void ConfigParser::openConfig()
    {
        m_stream.open(m_config_path, std::ios::in);
        if(std::ifstream::failbit == 0x04)
        {
//            printf("Failed to open config file at %s", CONFIG_PATH);
        }
    }

    void ConfigParser::parse()
    {
        std::string line;
        while(getline(m_stream, line))
        {
            std::cout << getToken(line) << std::endl;
        }
    }

    std::string ConfigParser::getToken(const std::string &line)
    {
        std::string token;
        bool prev_space = false;
        for(int i = 0; i < line.length(); i++) {
            char character = line[i];

            if(character == ' ')
            {
                prev_space = true;
                continue;
            }

            std::__detail::_Node_iterator<char, 1, 0> iterator;
            if((iterator = m_lexer.special_tokens.find(character)) != m_lexer.special_tokens.end())
            {
                if(!token.empty()) {
                    return token;
                }
                
                token = character;
                return token;
            }
            else if(prev_space)
            {
                printf("Unexpected whitespace at line 0 character %d\n", i);
            }
            
            token += character;
            prev_space = false;
        }

        if(token.empty())
            return token;
    }
}
