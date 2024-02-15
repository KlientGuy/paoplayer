#ifndef SONGCONTROLLER_H
#define SONGCONTROLLER_H

#include "PaConnector.h"

namespace songs
{
    class SongController {
    public:
        explicit SongController(pulse_audio::PaConnector *pa_connector);

        void enableVerbose();
        void enableQuiet();
        
        void setConfig();
        void setSingleMode();

        void setPlaylistUrl(const std::string &url);
        
        /**
         * Download songs from current playlist url starting at index_start
         * and ending at index_end
         * @param index_start 
         * @param index_end 
         * @return 0 on success, -1 if it cant access /tmp/music_widget directory
         * and -2 if yt-dlp exits unsuccessfully
         */
        int fetchSongsFromPlaylist(uint8_t index_start, uint8_t index_end);
        void prefetchSongs();
        int fetchSingle();
        
        /**
         * Skip to next track and if at the end of
         * downloaded songs call prefetchSongs() in child process;
         */
        void next();
        
        void play();
        void pause();
        
        /**
         * Go back a song
         */
        void previous();

        /**
         * Clears /tmp/music_widget
         */
        void removeExistingSongs(int up_to = 0);

    private:
        pulse_audio::PaConnector *pm_pa_connector;

        bool m_verbose = false;
        bool m_quiet = false;
        bool m_single_mode = false;

        std::string m_current_url;

        std::string m_current_song;
        uint8_t m_song_index = 0;
        uint8_t m_prefetch_count = 1;
        uint8_t m_max_index = 0;

        bool checkSongDir();
        std::string makeSongPattern() const;
        std::string findSongWithCurrentIndex();
        std::string getYtDlpCmd();
    };

#endif //SONGCONTROLLER_H
}
