#ifndef PULSE_AUDIO_H
#define PULSE_AUDIO_H

#include <iostream>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>
#include <functional>
#include <filesystem>

#include <pulse/mainloop.h>
#include <pulse/context.h>
#include <pulse/stream.h>
#include <pulse/error.h>
#include <pulse/volume.h>
#include <pulse/introspect.h>
#include <pulse/subscribe.h>

#include "MusicStateEnum.h"

#define SHM_PAGE_SIZE 4096
#define SHM_NAME "/music_widget"

#define WAV_HEADER_SIZE 44


namespace pulse_audio
{
    void context_set_state_callback(pa_context *context, void *userdata);
    void stream_state_callback(pa_stream *stream, void *userdata);
    void stream_write_callback(pa_stream *stream, size_t bytesCount, void *userdata);
    
    struct pc_shared_data {
        pid_t host_pid;
        uint8_t music_state;
        pa_cvolume volume;
    };

    class PaConnector {
    public:
        PaConnector();

        void enableDebug();
        void disableDebug();
        bool isDebug() const;

        /**
         * \brief Initializes main loop of pulse audio
         */
        void initMainloop();

        /**
         * \brief Starts a pulse audio main loop
         * \return Negative value on PA error, 0 otherwise
         */
        int startMainloop() const;

        /**
         * \brief Manualy iterates pulse audio main loop and handles state changes
         * \return PA iteration result
         */
        int iterateMainLoop();

        /**
         * \brief Closes pulse audio main loop
         */
        void freeMainloop() const;

        /**
         * \brief Sets up context for pulse audio to use
         * \param context_name user specified name of pulse audio context (shows in apps like pavucontrol)
         * \return 
         */
        bool createContext(const char *context_name);

        /**
         * \brief Creates a new PA stream, connects to it and sets the new stream as selected
         * for handling state changes
         */
        void connectStream();

        /**
         * \brief Opens a song to play and sets a pointer accessible via getCurrentSong()
         * \param fileName absolute path to the wav file
         * \return true if operation succeded, false otherwise
         */
        bool openSongToRead(const char *fileName);
        void closeCurrentSongFd();

        void addCurrentSongBytes(size_t bytes);
        void resetCurrentSongBytes();

        size_t getCurrentSongBytes() const;

        /**
         * \brief Get the current song file pointer
         * \return File pointer of a current song
         */
        FILE *getCurrentSong() const;

        void pause(pa_stream *stream);
        void play(pa_stream *stream);

        void setEndOfSongCallback(const std::function<void()> &callback);
        void executeEndOfSongCallback();

        /**
         * \brief Handles state changes on particular stream
         * \param stream stream to do action on
         */
        void handleStateChange(pa_stream *stream);

        uint8_t *getState() const;
        void setState(int state) const;
        void setStateAndExit(int state);

        bool createSharedMemory();
        bool attachSharedMemory();
        bool mapSharedMemory();
        void removeSharedMemory() const;

        pa_stream *getSelectedStream();
        void setSelectedStreamName(const char* name);
        void setSelectedStreamName();
        char* getSelectedStreamName();
        void paUnref(pa_operation* operation) const;
        
        void setVolume(double volume);
        void setVolumeObject(const pa_cvolume *vol_object);
        pa_cvolume* getVolumeObject();
        
        void changeVolume(double amount);
        
        void changeVolumeShared(double amount);
        void handleVolumeChange();

        static PaConnector *getInstance();
        void cleanup() const;
     
    private:
        bool m_debug = false;
        inline static PaConnector *pm_instance = nullptr;

        FILE *pm_current_song = nullptr;
        size_t m_current_song_byte_count = 0;
        std::function<void()> pm_end_of_song_callback;
        char* pm_selected_stream_name;

        pa_cvolume* m_volume = nullptr;

        int m_shm_fd{};
        /**
         * \brief Default state for stream is MS_DEFAULT defined in MusicStateEnum.h
         */
        uint8_t *pm_state{};
        pc_shared_data *pm_shared{};

        pa_mainloop *pm_main_loop{};
        pa_mainloop_api *pm_main_loop_api{};
        pa_context *pm_context{};
        pa_sample_spec pm_sample_spec{};
        pa_stream *pm_selected_stream{};
    };
}

#endif //PULSE_AUDIO_H
