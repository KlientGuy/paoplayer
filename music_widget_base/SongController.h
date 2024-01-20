#ifndef SONGCONTROLLER_H
#define SONGCONTROLLER_H
#include <cstdint>
#include <string>
#include <sys/stat.h>
#include <glob.h>

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
        void fetchSongsFromPlaylist(uint8_t index_start, uint8_t index_end);
        void next();
        void play();
        void pause();
        void previous();

        void removeExistingSongs();

    private:
        pulse_audio::PaConnector *pm_pa_connector;

        bool m_verbose = false;
        bool m_quiet = false;

        std::string m_current_url;

        std::string m_current_song;
        uint8_t m_song_index = 4;

        bool checkSongDir();
        std::string makeSongPattern() const;
        void findSongWithCurrentIndex();
    };

#endif //SONGCONTROLLER_H
}
