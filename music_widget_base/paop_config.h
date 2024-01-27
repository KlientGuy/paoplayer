#ifndef MUSIC_WIDGET_PAOP_CONFIG_H
#define MUSIC_WIDGET_PAOP_CONFIG_H

namespace config {
    struct paop_config {
        std::string default_playlist_url;
        int preload_size;
        bool save_previous;
    };
}

#endif //MUSIC_WIDGET_PAOP_CONFIG_H
