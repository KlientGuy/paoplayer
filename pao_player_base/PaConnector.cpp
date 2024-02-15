#include "PaConnector.h"
#include <iostream>
#include <cstring>
#include <pulse/stream.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <functional>
#include <pulse/error.h>
#include <pulse/volume.h>
#include <pulse/introspect.h>
#include <pulse/subscribe.h>

namespace pulse_audio
{
    PaConnector::PaConnector()
    {
        pm_instance = this;
        pm_selected_stream_name = "Music";
    }

    PaConnector *PaConnector::getInstance() { return pm_instance; }

    void PaConnector::enableDebug() { m_debug = true; }
    void PaConnector::disableDebug() { m_debug = false; }
    bool PaConnector::isDebug() const { return m_debug; }
    void PaConnector::startPaused() { m_start_paused = true; }
    bool PaConnector::startedPaused() { return m_start_paused; }

    void PaConnector::initMainloop()
    {
        pm_main_loop = pa_mainloop_new();
        pm_main_loop_api = pa_mainloop_get_api(pm_main_loop);
        pm_shared->host_pid = getpid();
    }

    void PaConnector::freeMainloop() const
    {
        pa_mainloop_free(pm_main_loop);
    }

    bool PaConnector::createContext(const char *context_name)
    {
        pm_context = pa_context_new(pm_main_loop_api, context_name);

        pa_context_set_state_callback(pm_context, context_set_state_callback, nullptr);
        if(pa_context_connect(pm_context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0)
            return false;

        return true;
    }

    int PaConnector::startMainloop() const
    {
        return pa_mainloop_run(this->pm_main_loop, nullptr);
    }

    int PaConnector::iterateMainLoop()
    {
        int retval = pa_mainloop_iterate(this->pm_main_loop, 0, nullptr);
        handleStateChange(pm_selected_stream);
        return retval;
    }

    void PaConnector::addCurrentSongBytes(size_t bytes) { this->m_current_song_byte_count += bytes; }
    void PaConnector::resetCurrentSongBytes() { this->m_current_song_byte_count = 0; }
    size_t PaConnector::getCurrentSongBytes() const { return this->m_current_song_byte_count; }

    FILE *PaConnector::getCurrentSong() const { return this->pm_current_song; }

    bool PaConnector::openSongToRead(const char *fileName)
    {
        if(FILE *file = fopen(fileName, "r"))
        {
            this->pm_current_song = file;
            
            //Gets rid of a that "pop" sound that plays when pushing a header data to pulse audio
            fseek(file, WAV_HEADER_SIZE * 2, SEEK_SET);
            return true;
        }

        switch(errno)
        {
            case ENOENT: fprintf(stderr, "File %s does not exist", fileName);
                break;
            case EACCES: fprintf(stderr, "You do not have read permissions on file %s", fileName);
                break;
            default: return false;
        }
        return false;
    }

    void PaConnector::closeCurrentSongFd()
    {
        fclose(pm_current_song);
    }
    
    void getDefaultVolume(pa_context* context, const pa_sink_input_info* info, int n, void* connector)
    {
        PaConnector* pa_connector = (PaConnector*) connector;
        
        if(info == nullptr)
            return;

        pa_connector->setVolumeObject(&info->volume);
        pa_operation* operation = pa_context_set_sink_input_volume(context, info->index, pa_connector->getVolumeObject(), nullptr, nullptr);
        pa_connector->paUnref(operation);

        if(pa_connector->isDebug())
        {
            std::cout << "INIT VOL: " << pa_sw_volume_to_linear(*info->volume.values) << std::endl;
            std::cout << "NEW VOL: " << pa_sw_volume_to_linear(*pa_connector->getVolumeObject()->values) << std::endl;
        }
    }
    
    void PaConnector::connectStream()
    {
        this->pm_sample_spec = {
            .format = PA_SAMPLE_S16LE,
            .rate = 48000,
            .channels = 2
        };

        pa_stream *stream = pa_stream_new(this->pm_context, pm_selected_stream_name.c_str(), &this->pm_sample_spec, nullptr);

        pa_stream_set_state_callback(stream, stream_state_callback, pm_context);
        pa_stream_connect_playback(stream, nullptr, nullptr, (pa_stream_flags) 0, nullptr, nullptr);

        pa_context_get_sink_input_info(pm_context, pa_stream_get_index(stream), getDefaultVolume, this);
        
        pm_selected_stream = stream;
    }
    

    bool PaConnector::createSharedMemory()
    {
        m_shm_fd = shm_open(SHM_NAME, O_EXCL | O_CREAT | O_RDWR, 0660);

        if(m_shm_fd == -1)
        {
            if(m_debug)
                std::cout << "PaConnector::createSharedMemory | shm_open() failed " << strerror(errno) << std::endl;

            removeSharedMemory();
            return false;
        }

        if(ftruncate(m_shm_fd, sizeof(pc_shared_data)) == -1)
        {
            if(m_debug)
                std::cout << "PaConnector::createSharedMemory | ftruncate() failed " << strerror(errno) << std::endl;

            removeSharedMemory();
            return false;
        }

        return mapSharedMemory();
    }

    bool PaConnector::attachSharedMemory()
    {
        m_shm_fd = shm_open(SHM_NAME, O_RDWR, 0660);

        if(m_shm_fd == -1)
        {
            if(m_debug)
                std::cout << "PaConnector::attachSharedMemory | shm_open() failed " << strerror(errno) << std::endl;

            removeSharedMemory();
            return false;
        }

        return mapSharedMemory();
    }

    bool PaConnector::mapSharedMemory()
    {
        pm_shared = (struct pc_shared_data*) mmap(nullptr, sizeof(pc_shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, m_shm_fd, 0);

        if(pm_shared == MAP_FAILED)
        {
            if(m_debug)
                std::cout << "PaConnector::createSharedMemory | mmap() failed " << strerror(errno) << std::endl;

            return false;
        }

        setState(MS_DEFAULT);

        return true;
    }


    void PaConnector::removeSharedMemory() const
    {
        munmap(pm_shared, sizeof(pc_shared_data));
        shm_unlink(SHM_NAME);
    }

    void PaConnector::handleStateChange(pa_stream *stream)
    {
        switch(pm_shared->music_state)
        {
            case MS_PAUSED:
                std::cout << "Paused" << std::endl;
                pause(stream);
                break;
            case MS_PLAYING:
                std::cout << "Resumed" << std::endl;
                play(stream);
                break;
            case MS_NEXT:
                executeEndOfSongCallback();
                break;
            case MS_VOL_CHNG:
                handleVolumeChange();
                break;
            case MS_DEFAULT:
                return;
            default:
                //Do nothing
                break;
        }

        setState(MS_DEFAULT);
    }

    pa_stream *PaConnector::getSelectedStream()
    {
        return pm_selected_stream;
    }

    void PaConnector::setSelectedStreamName(const std::string &name)
    {
//        pm_selected_stream_name = (char*)realloc(pm_selected_stream_name, strlen(name) * sizeof(char));
//        strcpy(pm_selected_stream_name, name);
        
        pm_selected_stream_name = name;
        
        if(pm_selected_stream != nullptr)
            paUnref(pa_stream_set_name(pm_selected_stream, pm_selected_stream_name.c_str(), nullptr, nullptr));
    }

    void PaConnector::setSelectedStreamName()
    {
        if(pm_selected_stream != nullptr)
        {
            paUnref(pa_stream_set_name(pm_selected_stream, pm_selected_stream_name.c_str(), nullptr, nullptr));
        }
    }

    std::string PaConnector::getSelectedStreamName()
    {
        return pm_selected_stream_name;
    }

    void PaConnector::setCurrentlyPlaying(const std::string *title)
    {
        memset(pm_shared->currently_playing, '\0', 200);
        memcpy(pm_shared->currently_playing, title->c_str(), title->length());
    }

    void PaConnector::printCurrentlyPlayingShared()
    {
        if(attachSharedMemory())
        {
            std::cout << "  " << pm_shared->currently_playing << "  " << std::endl;
            munmap(pm_shared, sizeof(pc_shared_data));
        }
    }

    void PaConnector::paUnref(pa_operation *operation) const
    {
        if(operation != nullptr)
            pa_operation_unref(operation);
    }


    void PaConnector::pause(pa_stream *stream)
    {
        pa_stream_cork(stream, 1, nullptr, nullptr);
    }

    void PaConnector::play(pa_stream *stream)
    {
        pa_stream_cork(stream, 0, nullptr, nullptr);
    }

    uint8_t *PaConnector::getState() const { return &this->pm_shared->music_state; }

    void PaConnector::setState(const int state) const
    {
        pm_shared->music_state = state;
    }

    void PaConnector::setStateAndExit(int state)
    {
        attachSharedMemory();
        pm_shared->music_state = state;
        munmap(pm_shared, sizeof(pc_shared_data));
        exit(0);
    }

    void PaConnector::cleanup() const
    {
        pa_mainloop_free(this->pm_main_loop);
        removeSharedMemory();
        delete pm_config;
    }

    void PaConnector::executeEndOfSongCallback()
    {
        pm_end_of_song_callback();
    }

    void PaConnector::setEndOfSongCallback(const std::function<void()> &callback)
    {
        pm_end_of_song_callback = callback;
    }

    void PaConnector::setVolume(double volume)
    {
        pa_cvolume_set(&pm_shared->volume, 2, pa_sw_volume_from_linear(volume));

        if(pm_shared->host_pid == getpid()) {
            pa_context_set_sink_input_volume(pm_context, pa_stream_get_index(pm_selected_stream), &pm_shared->volume, nullptr, nullptr);
        }
    }

    void PaConnector::setVolumeObject(const pa_cvolume *vol_object)
    {
        pm_shared->volume = *vol_object;
    }
    
    pa_cvolume* PaConnector::getVolumeObject() 
    {
        return &pm_shared->volume;
    }

    void PaConnector::changeVolume(double amount)
    {
        double curr_vol = pa_sw_volume_to_linear(*pm_shared->volume.values);
        double new_volume = curr_vol + amount / 100;

        if(isDebug())
        {
            std::string action = amount < 0 ? "DECREASE" : "INCREASE";
            std::cout << action << std::endl;
            std::cout << "Current Volume: " << curr_vol << std::endl;
            std::cout << "ACCUM: " << curr_vol + amount << std::endl;
        }

        if((amount < 0 && new_volume > 0) || (amount > 0 && new_volume < 1)) {
            setVolume(new_volume);
        }
        else {
            setVolume(0);
        }       
    }

    void PaConnector::changeVolumeShared(double amount)
    {
        if(attachSharedMemory())
        {
            changeVolume(amount);
            setState(MS_VOL_CHNG);
            munmap(pm_shared, sizeof(pc_shared_data));
        }
    }

    void PaConnector::handleVolumeChange()
    {
        double curr_vol = pa_sw_volume_to_linear(*pm_shared->volume.values);
        setVolume(curr_vol);
    }

    void PaConnector::setConfig(config::paop_config *config)
    {
        pm_config = config;
    }

    config::paop_config *PaConnector::getConfig()
    {
        return pm_config;
    }

    void subCb(pa_context* context, pa_subscription_event_type_t event, uint32_t index, void* userdata)
    {
        //On volume change;
        if((event & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SINK_INPUT && (event & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_CHANGE)
        {
            PaConnector* pa_connector = PaConnector::getInstance();
            pa_operation* operation = pa_context_get_sink_input_info(context, pa_stream_get_index(pa_connector->getSelectedStream()), getDefaultVolume, pa_connector);
            pa_connector->paUnref(operation);
        }
    }

    void context_set_state_callback(pa_context *context, void *userdata)
    {
        PaConnector *paConnector = PaConnector::getInstance();
        const pa_context_state_t state = pa_context_get_state(context);

        if(paConnector->isDebug())
        {
            switch(state)
            {
                case PA_CONTEXT_UNCONNECTED:
                    printf("PA_CONTEXT_UNCONNECTED\n");
                    break;
                case PA_CONTEXT_CONNECTING:
                    printf("PA_CONTEXT_CONNECTING\n");
                    break;
                case PA_CONTEXT_AUTHORIZING:
                    printf("PA_CONTEXT_AUTHORIZING\n");
                    break;
                case PA_CONTEXT_SETTING_NAME:
                    printf("PA_CONTEXT_SETTING_NAME\n");
                    break;
                case PA_CONTEXT_READY:
                    printf("PA_CONTEXT_READY\n");
                    goto context_ready;
                case PA_CONTEXT_FAILED:
                    printf("PA_CONTEXT_FAILED\n");
                    std::cout << pa_strerror(pa_context_errno(context)) << std::endl;
                    paConnector->removeSharedMemory();
                    exit(-1);
                case PA_CONTEXT_TERMINATED:
                    printf("PA_CONTEXT_TERMINATED\n");
                    break;
                default:
                    std::cout << state << std::endl;
                    break;
            }
        }
        else
        {
            switch(state)
            {
                case PA_CONTEXT_READY: goto context_ready;
                case PA_CONTEXT_FAILED:
                    std::cout << "Context creation failed with code " << pa_strerror(pa_context_errno(context)) <<
                            std::endl;
                    paConnector->removeSharedMemory();
                    exit(-1);
                default:
                    //Do nothing
                    return;
            }
        }
        
    context_ready:
        paConnector->connectStream();
        pa_operation* operation = pa_context_subscribe(context,  PA_SUBSCRIPTION_MASK_SINK_INPUT, nullptr, nullptr);
        pa_context_set_subscribe_callback(context, subCb, nullptr);

        paConnector->paUnref(operation);
    }


    void stream_write_callback(pa_stream *stream, size_t bytesCount, void *userdata)
    {
        uint8_t buffer[bytesCount];
        PaConnector *paConnector = PaConnector::getInstance();

        paConnector->handleStateChange(stream);

        FILE *currentSong = paConnector->getCurrentSong();
        
        if(currentSong == nullptr)
            return;
        
        size_t bytes = fread(buffer, 1, sizeof(buffer), currentSong);

        paConnector->addCurrentSongBytes(bytes);

        if(paConnector->isDebug())
        {
            // std::cout << bytesCount << " : FREE Bytes" << std::endl;
            // std::cout << bytes << " : BYTES READ" << std::endl;
        }

        if(feof(currentSong) != 0)
        {
            paConnector->resetCurrentSongBytes();
            paConnector->closeCurrentSongFd();
            // paConnector->cleanup();
            paConnector->executeEndOfSongCallback();
            return;
        }

        const int written = pa_stream_write(stream, buffer, bytesCount, nullptr, 0, PA_SEEK_RELATIVE);

//        if(written < 0)
//            exit(0);

        pa_operation* operation = pa_stream_drain(stream, nullptr, nullptr);

        paConnector->paUnref(operation);
    }

    void stream_state_callback(pa_stream *stream, void *context)
    {
        const pa_stream_state_t state = pa_stream_get_state(stream);
        PaConnector *paConnector = PaConnector::getInstance();

        if(paConnector->isDebug()) goto handle_debug;
        else goto handle_no_debug;

    handle_debug:
        switch(state)
        {
            case PA_STREAM_UNCONNECTED:
                printf("PA_STREAM_UNCONNECTED\n");
                break;
            case PA_STREAM_CREATING:
                printf("PA_STREAM_CREATING\n");
                break;
            case PA_STREAM_READY:
                printf("PA_STREAM_READY\n");
                pa_context_get_sink_input_info((pa_context*)context, pa_stream_get_index(stream), getDefaultVolume, (void*)paConnector);
                std::cout << pa_strerror(pa_context_errno((pa_context*)context)) << std::endl;
                pa_stream_set_write_callback(stream, stream_write_callback, nullptr);
                if(paConnector->startedPaused()) {
                    pa_stream_cork(stream, 1, nullptr, nullptr);
                }
                break;
            case PA_STREAM_FAILED:
                printf("PA_STREAM_FAILED\n");
                paConnector->removeSharedMemory();
                break;
            case PA_STREAM_TERMINATED:
                printf("PA_STREAM_TERMINATED\n");
                break;
            default:
                std::cout << state << std::endl;
                break;
        }

    handle_no_debug:
        switch(state)
        {
            case PA_STREAM_READY:
                {
                    pa_operation* operation;
                    pa_stream_set_write_callback(stream, stream_write_callback, nullptr);
                    if(paConnector->startedPaused()) {
                        pa_stream_cork(stream, 1, nullptr, nullptr);
                    }
                    operation = pa_context_get_sink_input_info((pa_context*) context, pa_stream_get_index(stream), getDefaultVolume, (void*) paConnector);
                    paConnector->paUnref(operation);
                }
                break;
            case PA_STREAM_FAILED:
                std::cout << "Pulse Audio m_stream failed" << std::endl;
                paConnector->removeSharedMemory();
                break;
            default:
                //Do nothing
                break;
        }
    }
}
