#include "ConfigParser.h"

namespace config {
    ConfigParser::ConfigParser()
    {
        pm_config = new paop_config;
        m_config_path = std::string(getenv("HOME")) + "/.config/paoplayer/config";
        openConfig();
    }
    
    void ConfigParser::openConfig()
    {
        m_stream.open(m_config_path, std::ios::in);
        if(m_stream.fail())
        {
            printf("Failed to open config file at %s: %s\n", m_config_path.c_str(), strerror(errno));
        }
    }

    paop_config* ConfigParser::parse()
    {
        std::string line, err_message;;
        int token_type, line_count = 1;
        
        while(getline(m_stream, line))
        {
            int line_index = -1;
            token_type = TOKEN_COMMENT;
            long line_len = (long)line.length();
            std::string key;
            
            while(line_index < line_len)
            {
                std::string token = getToken(line, line_index, token_type, err_message);
                
                switch(token_type) {
                    case TOKEN_KEY:
                        key = token;
                        line_index--;
                        break;
                    case TOKEN_SPECIAL:
                        token_type = TOKEN_VALUE;
                        break;
                    case TOKEN_VALUE:
                        setConfigProps(key, token);
                        break;
                    case TOKEN_INVALID:
                    case TOKEN_COMMENT:
                        goto endline;
                    default:
                        //Do nothing
                        break;
                }
            }
            
        endline:
            line_count++;
            
            if(token_type == TOKEN_INVALID)
                break;
        }
        if(token_type == TOKEN_INVALID)
        {
            std::cerr << err_message << " line " << std::to_string(line_count) << std::endl;
            std::cerr << "Here: " << line << std::endl;
        }
        
        return pm_config;
    }

    std::string ConfigParser::getToken(const std::string &line, int &line_index, int &token_type, std::string &error_message)
    {
        std::string token;
        bool prev_space = false;
        int i;

        if(line.empty())
        {
            token_type = TOKEN_COMMENT;
            return "";
        }

        if(line[line_index + 1] == '#')
        {
            token_type = TOKEN_COMMENT;
            return "";
        }

        for(i = line_index + 1; i < line.length(); i++) {
            char character = line[i];

            if(character == ' ')
            {
                prev_space = true;
                continue;
            }

            if(character == '#') {
                break;
            }

            if(token_type == TOKEN_VALUE)
            {
                token += character;
                prev_space = false;
                continue;
            }

            if(m_lexer.special_tokens.find(character) != m_lexer.special_tokens.end())
            {
                if(!token.empty()) {
                    token_type = TOKEN_KEY;
                    break;
                }

                token_type = TOKEN_SPECIAL;
                token = character;
                break;
            }
            else if(prev_space && token_type != TOKEN_SPECIAL)
            {
                token_type = TOKEN_INVALID;
                error_message = "Unexpected whitespace at character " + std::to_string(i);
                break;
            }
            
            token += character;
            prev_space = false;
        }

        line_index = i;
        token.erase(token.begin(), std::find_if(token.begin(), token.end(), [](unsigned char chr) {
            return !std::iswspace(chr);
        }));
        
        token.erase(std::find_if(token.rbegin(), token.rend(), [](unsigned char chr) {
            return !std::iswspace(chr);
        }).base(), token.end());
        
        return token;
    }

    void ConfigParser::setConfigProps(const std::string &key, std::string &value)
    {
        if(m_config_map.find(key) == m_config_map.end())
        {
            std::cout << "Unrecognized option " << key << std::endl;
            return;
        }

        switch(m_config_map.at(key)) {
            case CONF_DEFAULT_PLAYLIST_URL:
                pm_config->default_playlist_url = value;
                break;
            case CONF_PRELOAD_COUNT:
                pm_config->preload_size = std::stoi(value);
                break;
            case CONF_SAVE_PREVIOUS:
            {
                bool val;
                std::istringstream stream(value);
                stream >> std::boolalpha >> val;
                pm_config->save_previous = val;
                break;
            }
            case CONF_SONG_DIR:
                if(value[value.length() - 1] != '/')
                    value += '/';

                pm_config->song_dir = value;
                break;
            case CONF_BROWSER_COOKIES:
                pm_config->browser_cookies = value;
                break;
        }
    }
}
