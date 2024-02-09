#include "SongController.h"
#include "paop_config.h"
#include <thread>
#include <locale>
#include <algorithm>
#include <iostream>

namespace songs
{
    SongController::SongController(pulse_audio::PaConnector *pa_connector)
    {
        pm_pa_connector = pa_connector;
    }

    void SongController::setConfig()
    {
        config::paop_config* config_instance = pm_pa_connector->getConfig();
        if(int size = config_instance->preload_size) {
            m_prefetch_count = size;
        }

        pm_pa_connector->setEndOfSongCallback([this] { 
            std::cout << "NEXT CALLBACK" << std::endl;
            next(); 
        });
    }

    void SongController::setSingleMode()
    {
        m_single_mode = true;

        pm_pa_connector->setEndOfSongCallback([this] {
            std::cout << "CLEANUP CALLBACK" << std::endl;
            pm_pa_connector->cleanup();
        });
    }

    void SongController::setPlaylistUrl(const std::string &url)
    {
        m_current_url = url;
    }

    int SongController::fetchSongsFromPlaylist(const uint8_t index_start, const uint8_t index_end)
    {
        if(!checkSongDir())
        {
            if(!m_quiet)
                std::cout << "Could not create a directory at " << pm_pa_connector->getConfig()->song_dir << std::endl;

            return -1;
        }
        
        std::string command = getYtDlpCmd();

        char *prepare_command = (char *) malloc(command.length() + m_current_url.size() * sizeof(char));

        sprintf(prepare_command, command.c_str(), index_start, index_end, m_current_url.c_str());

        if(m_verbose)
            std::cout << "Downloading with command \033[32m\n" << prepare_command << "\033[0m" <<  std::endl;

        if(system(prepare_command) == 256 && !m_quiet)
        {
            std::cout << "yt-dlp exited with error" << std::endl;
            return -2;
        }

		if(m_song_index < 1) {
        	m_song_index = index_start - 1;
		}
        m_max_index = index_end;

        free(prepare_command);
        return 0;
    }

    int SongController::fetchSingle()
    {
        if(!checkSongDir())
        {
            if(!m_quiet)
                std::cout << "Could not create a directory at " << pm_pa_connector->getConfig()->song_dir << std::endl;

            return -1;
        }
        
        std::string command = getYtDlpCmd();

        char *prepare_command = (char *) malloc(command.length() + m_current_url.length() * sizeof(char));

        sprintf(prepare_command, command.c_str(), m_current_url.c_str());

        if(m_verbose)
            std::cout << "Downloading with command \033[32m\n" << prepare_command << "\033[0m" <<  std::endl;

        if(system(prepare_command) == 256 && !m_quiet)
        {
            std::cout << "yt-dlp exited with error" << std::endl;
            return -2;
        }

        free(prepare_command);
        return 0;
    }

    void SongController::prefetchSongs()
    {
        fetchSongsFromPlaylist( m_max_index + 1, m_max_index + m_prefetch_count);
    }

    bool SongController::checkSongDir()
    {
        struct stat dir_info{};
        const char* song_dir = pm_pa_connector->getConfig()->song_dir.c_str();
        if(stat(song_dir, &dir_info) != 0)
        {
            if(errno == ENOENT)
            {
                if(mkdir(song_dir, S_IRWXU | S_IRGRP | S_IWGRP) == 0)
                    return true;
            }

            if(!m_quiet)
                std::cout << "Cannot access " << song_dir << " " << strerror(errno) << std::endl;

            return false;
        }

        if(dir_info.st_mode & S_IFDIR)
            return true;

        return false;
    }

    void SongController::play()
    {
        pm_pa_connector->play(pm_pa_connector->getSelectedStream());
    }

    void SongController::pause()
    {
        pm_pa_connector->pause(pm_pa_connector->getSelectedStream());
    }

    void SongController::next()
    {
        m_song_index++;
        
        std::string name = findSongWithCurrentIndex();
        pm_pa_connector->setSelectedStreamName(name);

        if(!pm_pa_connector->getConfig()->save_previous && m_song_index > 1) {
            removeExistingSongs(m_song_index - 1);
        }
        
        std::string playing = "\uF58F  " + name;
        
        std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

        std::wstring convert_string = convert.from_bytes(playing);
        
        if(convert_string.length() > 50) {
            convert_string = convert_string.substr(0, 47) + L"...";
        }
        
        playing = convert.to_bytes(convert_string);

        pm_pa_connector->setCurrentlyPlaying(&playing);
        
        if(m_song_index == m_max_index)
        {
            std::thread download_thread(&SongController::prefetchSongs, this);
            download_thread.detach();
        }
    }

    void SongController::previous()
    {
        if(--m_song_index <= 0)
        {
            next();
            return;
        }

//        findSongWithCurrentIndex();
    }

    std::string SongController::findSongWithCurrentIndex()
    {
        const std::string path = makeSongPattern();

        glob_t buff;

        glob(path.c_str(), 0, nullptr, &buff);

        if(buff.gl_pathc > 0)
        {
            pm_pa_connector->openSongToRead(buff.gl_pathv[0]);
        }
        else if(!m_quiet)
        {
            std::cout << "No song found with index: " << std::to_string(m_song_index) << std::endl;
            return "";
        }

        if(!m_quiet)
        {
            for(int i = 0; i < buff.gl_pathc; i++)
            {
                std::cout << "Found: " << buff.gl_pathv[i] << std::endl;
            }
        }

        std::string out = buff.gl_pathv[0];
        std::string* song_dir = &pm_pa_connector->getConfig()->song_dir;
        
        out.erase(0, song_dir->length());
        std::string suffix = "_index_" + std::to_string(m_song_index) + ".wav";
        out.erase(out.length() - suffix.length(), suffix.length());

        m_current_song = out;
        
        globfree(&buff);
        return out;
    }

    std::string SongController::makeSongPattern() const
    {
        return pm_pa_connector->getConfig()->song_dir + "*_index_" + std::to_string(m_song_index) + ".wav";
    }

    void SongController::removeExistingSongs(const int up_to)
    {
        std::string song_dir = pm_pa_connector->getConfig()->song_dir;
        if(!m_quiet)
        {
            std::cout << "Removing from " << song_dir << "*.wav";
            
            if(up_to > 0)
                std::cout << " up to index " << std::to_string(up_to);
            
            std::cout << std::endl;
        }

        glob_t buffer;

        glob((song_dir + "*").c_str(), 0, nullptr, &buffer);

        if(m_verbose && buffer.gl_pathc <= 0)
            std::cout << "Nothing to remove" << std::endl;

        int first_index = 0x0FFFFFFF;
        if(up_to > 0)
        {
            std::sort(buffer.gl_pathv, buffer.gl_pathv + buffer.gl_pathc, [&first_index](const char* first, const char* second){
                std::string firstStr = first;
                std::string secondStr = second;
                
                unsigned long first_start = firstStr.rfind('_');
                unsigned long first_end = firstStr.rfind('.');

                unsigned long second_start = secondStr.rfind('_');
                unsigned long second_end = secondStr.rfind('.');

                int index1 = std::stoi(firstStr.substr(first_start + 1, first_end - first_start));
                int index2 = std::stoi(secondStr.substr(second_start + 1, second_end - second_start));
                
                if(first_index > index1) {
                    first_index = index1;
                }
                
                return index1 < index2;
            });
        }

        for(int i = 0; i < buffer.gl_pathc; i++)
        {
            if(up_to > 0 && first_index == up_to)
                break;
            
            if(unlink(buffer.gl_pathv[i]) == -1 && !m_quiet) {
                std::cout << strerror(errno) << std::endl;
            }
            else if(m_verbose) {
                std::cout << "Removed " << buffer.gl_pathv[i] << std::endl;
            }
            
            first_index++;
        }

        globfree(&buffer);
    }


    void SongController::enableVerbose()
    {
        m_verbose = true;
    }

    void SongController::enableQuiet()
    {
        m_quiet = true;
    }

    std::string SongController::getYtDlpCmd()
    {
        config::paop_config* config_instance = pm_pa_connector->getConfig();
        std::string cmd = "yt-dlp -q -o ";
        cmd += config_instance->song_dir;

        if(m_single_mode) {
            cmd += "\"%%(title)s_index_1\"";
        }
        else {
            cmd += "\"%%(title)s_index_%%(playlist_index)s\"";
            cmd += " -I %d:%d";
        }


        if(!config_instance->browser_cookies.empty()) {
            cmd += " --cookies-from-browser " + config_instance->browser_cookies;
        }

        cmd += " -x --audio-format wav %s";

        return cmd;
    }
}
