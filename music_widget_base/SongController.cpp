#include "SongController.h"

namespace songs
{
    SongController::SongController(pulse_audio::PaConnector *pa_connector)
    {
        pm_pa_connector = pa_connector;
        pa_connector->setEndOfSongCallback([this] { next(); });
    }

    void SongController::setPlaylistUrl(const std::string &url)
    {
        m_current_url = url;
    }

    void SongController::fetchSongsFromPlaylist(const uint8_t index_start, const uint8_t index_end)
    {
        if(!checkSongDir())
        {
            if(!m_quiet)
                std::cout << "Could not create a directory at " << SONG_DIR << std::endl;

            return;
        }

        char *prepare_command = (char *) malloc(sizeof(YT_DLP_CMD) + m_current_url.size() * sizeof(char));

        sprintf(prepare_command, YT_DLP_CMD, index_start, index_end, m_current_url.c_str());

        if(m_verbose)
            std::cout << "Downloading with command " << prepare_command << std::endl;

        if(system(prepare_command) == 256 && !m_quiet)
        {
            std::cout << "yt-dlp exited with error" << std::endl;
        }

        m_song_index = index_start;

        free(prepare_command);
    }

    bool SongController::checkSongDir()
    {
        struct stat dir_info{};
        if(stat(SONG_DIR, &dir_info) != 0)
        {
            if(errno == ENOENT)
            {
                if(mkdir(SONG_DIR, S_IRWXU | S_IRGRP | S_IWGRP) == 0)
                    return true;
            }

            if(!m_quiet)
                std::cout << "Cannot access " << SONG_DIR << " " << strerror(errno) << std::endl;

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
        findSongWithCurrentIndex();
    }

    void SongController::previous()
    {
        if(--m_song_index <= 0)
        {
            next();
            return;
        }

        findSongWithCurrentIndex();
    }

    void SongController::findSongWithCurrentIndex()
    {
        const std::string path = makeSongPattern();

        glob_t buff;

        glob(path.c_str(), 0, nullptr, &buff);

        if(buff.gl_pathc > 0)
        {
            pm_pa_connector->openSongToRead(buff.gl_pathv[0]);
            m_current_song = buff.gl_pathv[0];
        }
        else if(!m_quiet)
        {
            std::cout << "No song found with index: " << m_song_index << std::endl;
        }

        if(!m_quiet)
        {
            for(int i = 0; i < buff.gl_pathc; i++)
            {
                std::cout << "Found: " << buff.gl_pathv[i] << std::endl;
            }
        }
    }

    std::string SongController::makeSongPattern() const
    {
        return std::string(SONG_DIR) + "*_index_" + std::to_string(m_song_index) + std::string(".wav");
    }

    void SongController::removeExistingSongs()
    {
        if(!m_quiet)
            std::cout << "Removing from " << SONG_DIR "*.wav" << std::endl;

        glob_t buffer;

        glob(SONG_DIR "*", 0, nullptr, &buffer);

        if(m_verbose && buffer.gl_pathc <= 0)
            std::cout << "Nothing to remove" << std::endl;

        for(int i = 0; i < buffer.gl_pathc; i++)
        {
            if(unlink(buffer.gl_pathv[i]) == -1 && !m_quiet)
            {
                std::cout << strerror(errno) << std::endl;
            }
            else if(m_verbose)
            {
                std::cout << "Removed " << buffer.gl_pathv[i] << std::endl;
            }
        }
    }


    void SongController::enableVerbose()
    {
        m_verbose = true;
    }

    void SongController::enableQuiet()
    {
        m_quiet = true;
    }
}
