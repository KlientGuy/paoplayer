#ifndef MUSIC_WIDGET_PAOP_CONFIG_H
#define MUSIC_WIDGET_PAOP_CONFIG_H

#include <string>

namespace config {
    struct paop_config {
        std::string default_playlist_url;
        int preload_size;
        bool save_previous;
        std::string song_dir;
        std::string browser_cookies;
    };
}

#endif //MUSIC_WIDGET_PAOP_CONFIG_H
