#include <bitset>
#include <iostream>
#include <getopt.h>
#include <signal.h>
#include <cmath>

#include "SongController.h"
#include "lib/OP_MASKS.h"
#include "ConfigParser.h"

#define MILISECOND 1000

int ops_mask = OPS_MASK;

config::ConfigParser config_parser;
pulse_audio::PaConnector pa_connector;
songs::SongController song_controller(&pa_connector);
config::paop_config* config_instance;

bool online = false;

void processArgv(const int argc, char *argv[])
{
    int optCode;
    const char *shortOptions = ":PptqvD ?U: :cn ?u: ?d: -g";
    const option longOptions[] = {
        {"play", no_argument, nullptr, 'P'},
        {"pause", no_argument, nullptr, 'p'},
        {"quiet", no_argument, nullptr, 'q'},
        {"verbose", no_argument, nullptr, 'v'},
        {"debug", no_argument, nullptr, 'd'},
        {"from-url", no_argument, nullptr, 'U'},
        {"clear-existing", no_argument, nullptr, 'c'},
        {"next", no_argument, nullptr, 'n'},
        {"volume-up", no_argument, nullptr, 'u'},
        {"volume-down", no_argument, nullptr, 'd'},
        {"currently-playing", no_argument, nullptr, 'g'},
        {nullptr, no_argument, nullptr, 0} //Segfaults on unrecognized option
    };
    
    double vol_amount = 0;

    while((optCode = getopt_long(argc, argv, shortOptions, longOptions, nullptr)) != -1)
    {
        switch(optCode)
        {
            case 'P': ops_mask |= OPS_PLAY;
                break;
            case 'p': ops_mask |= OPS_PAUSE;
                break;
            case 'n': ops_mask |= OPS_NEXT;
                break;
            case 'u':
                ops_mask |= OPS_VOL_UP;
                vol_amount = std::stod(optarg);
                if(vol_amount <= 0)
                    std::cout << "Invalid argument passed for -u; Expecting positive floating point number";
                break;
            case 'd':
                ops_mask |= OPS_VOL_DOWN;
                vol_amount = std::stod(optarg);
                if(vol_amount <= 0)
                    std::cout << "Invalid argument passed for -d; Expecting positive floating point number";
                break;
            case 'U': ops_mask |= OPS_FROM_URL;
                song_controller.setPlaylistUrl(optarg);
                break;
            case 'g': ops_mask |= OPS_GET_PLAYING;
                break;
            case 'c': ops_mask |= OPS_CLEAR_EXISTING;
                break;
            case 'q': ops_mask |= OPS_QUIET;
                break;
            case 'v': ops_mask |= OPS_VERBOSE;
                break;
            case 'D': ops_mask |= OPS_DEBUG;
                break;
            case ':':
                if(optopt == 'U')
                {
                    printf("No arg passed to -U using URL from config\n");
//                    std::cout << config_instance->default_playlist_url << std::endl;
                    song_controller.setPlaylistUrl(config_instance->default_playlist_url);
                    ops_mask |= OPS_FROM_URL;
                    break;
                }
                printf("Music Widget: option %s needs and argument\n", argv[optopt - 1]);
                exit(-1);
            default:
                pa_connector.removeSharedMemory();
                printf("Music Widget: Unrecognized option: %s\n", argv[optopt - 1]);
                exit(-1);
        }
    }

    if((ops_mask & OPS_DEBUG) == OPS_DEBUG) {
        pa_connector.enableDebug();
    }

    if((ops_mask & OPS_VERBOSE) == OPS_VERBOSE)
    {
        song_controller.enableVerbose();

        if((ops_mask & OPS_QUIET) == OPS_QUIET)
            std::cout << "Music Widget: -v passed ignoring -q" << std::endl << std::endl;
    }

    if((ops_mask & OPS_QUIET) == OPS_QUIET && ((ops_mask & OPS_VERBOSE) != OPS_VERBOSE)) {
        song_controller.enableQuiet();
    }

    if((ops_mask & OPS_VOL_UP) == OPS_VOL_UP && (ops_mask & OPS_VOL_DOWN) == OPS_VOL_DOWN)
    {
        std::cout << "Music Widget: -d and -u are mutually exclusive" << std::endl;
    }

    if((ops_mask & OPS_GET_PLAYING) == OPS_GET_PLAYING) {
        pa_connector.printCurrentlyPlayingShared();
        exit(0);
    }
    
    if((ops_mask & OPS_VOL_UP) == OPS_VOL_UP) {
        pa_connector.changeVolumeShared(vol_amount);
        exit(0);
    }
    
    if((ops_mask & OPS_VOL_DOWN) == OPS_VOL_DOWN) {
        pa_connector.changeVolumeShared(-vol_amount);
        exit(0);
    }

    if((ops_mask & OPS_PLAY) == OPS_PLAY) {
        pa_connector.setStateAndExit(MS_PLAYING);
    }

    if((ops_mask & OPS_PAUSE) == OPS_PAUSE) {
        pa_connector.setStateAndExit(MS_PAUSED);
    }

    if((ops_mask & OPS_NEXT) == OPS_NEXT) {
        pa_connector.setStateAndExit(MS_NEXT);
    }

    if((ops_mask & OPS_CLEAR_EXISTING) == OPS_CLEAR_EXISTING) {
        song_controller.removeExistingSongs();
    }

    if((ops_mask & OPS_FROM_URL) == OPS_FROM_URL)
    {
        int size = 1;

        if(config_instance->preload_size) {
            size = config_instance->preload_size;
        }
        
        song_controller.fetchSongsFromPlaylist(1, size);
        online = true;
    }
}

void sigsegvCleanup(int id)
{
    pa_connector.removeSharedMemory();
    exit(1);
}

void sigtermCleanup(int id)
{
    if(pa_connector.isDebug())
        std::cout << strsignal(id) << " received, cleaning up..." << std::endl;

    pa_connector.cleanup();
    exit(0);
}

void setupSignals()
{
    signal(SIGSEGV, sigsegvCleanup);
    signal(SIGTERM, sigtermCleanup);
    signal(SIGINT, sigtermCleanup);
    signal(SIGQUIT, sigtermCleanup);
    signal(SIGABRT, sigtermCleanup);
}

int main(int argc, char *argv[])
{
    config_instance = config_parser.parse();
    pa_connector.setConfig(config_instance);
    song_controller.setConfig();
    //std::cout << "ADAM" << std::endl;

    //exit(0);
    setupSignals();

    int err_code = 0;

    processArgv(argc, argv);

    if(!pa_connector.createSharedMemory())
    {
        pa_connector.removeSharedMemory();
        exit(1);
    }

    char *songPath = argv[1];

    if(songPath == nullptr && !online)
    {
        std::cerr << "No file path passed, exiting..." << std::endl;
        pa_connector.removeSharedMemory();
        exit(1);
    }

    pa_connector.initMainloop();

    if(!pa_connector.createContext("Pao player"))
    {
        std::cerr << "pulse_audio.create_context() failed!";
        pa_connector.removeSharedMemory();
        exit(1);
    }
    
    if(online) {
        song_controller.next();
    }
    else if(!pa_connector.openSongToRead(argv[1]))
    {
        pa_connector.removeSharedMemory();
        exit(1);
    }

    while(true)
    {
        err_code = pa_connector.iterateMainLoop();
        if(err_code < 0)
        {
            std::cerr << "pulse_audio.iterate_main_loop() failed with code " << pa_strerror(err_code);
            pa_connector.cleanup();
            exit(1);
        }
        usleep(MILISECOND);
    }
}
