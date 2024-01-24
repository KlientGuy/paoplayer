#ifndef SONGCONTROLLER_H
#define SONGCONTROLLER_H
#include <cstdint>
#include <string>
#include <sys/stat.h>
#include <glob.h>
#include <thread>

#include "PaConnector.h"

#define SONG_DIR "/tmp/music_widget/"
#define YT_DLP_CMD ("yt-dlp -o \"" SONG_DIR "%(title)s_index_%(playlist_index)s\" --cookies-from-browser firefox -x --audio-format wav -I %d:%d %s")

namespace songs
{
    class SongController {
    public:
        explicit SongController(pulse_audio::PaConnector *pa_connector);

        void enableVerbose();
        void enableQuiet();

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
        void removeExistingSongs();

    private:
        pulse_audio::PaConnector *pm_pa_connector;

        bool m_verbose = false;
        bool m_quiet = false;

        std::string m_current_url;

        std::string m_current_song;
        uint8_t m_song_index = 0;
        uint8_t m_prefetch_count = 1;
        uint8_t m_max_index = 0;

        bool checkSongDir();
        std::string makeSongPattern() const;
        std::string findSongWithCurrentIndex();
    };

#endif //SONGCONTROLLER_H
}
