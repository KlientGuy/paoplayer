#include <bitset>
#include <iostream>
#include <getopt.h>
#include <signal.h>

#include "SongController.h"
#include "lib/OP_MASKS.h"

#define MILISECOND 1000

int ops_mask = OPS_MASK;

pulse_audio::PaConnector pa_connector;
songs::SongController song_controller(&pa_connector);

bool online = false;

void processArgv(const int argc, char *argv[])
{
    int optCode;
    const char *shortOptions = ":PptqvD ?U: :cn";
    const option longOptions[] = {
        {"play", no_argument, nullptr, 'P'},
        {"pause", no_argument, nullptr, 'p'},
        {"quiet", no_argument, nullptr, 'q'},
        {"verbose", no_argument, nullptr, 'v'},
        {"debug", no_argument, nullptr, 'd'},
        {"from-url", no_argument, nullptr, 'U'},
        {"clear-existing", no_argument, nullptr, 'c'},
        {"next", no_argument, nullptr, 'n'},
        {nullptr, no_argument, nullptr, 0} //Segfaults on unrecognized option
    };

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
            case 'U': ops_mask |= OPS_FROM_URL;
                song_controller.setPlaylistUrl(optarg);
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
                printf("Music Widget: option %s needs and argument\n", argv[optind - 1]);
                exit(-1);
            default:
                pa_connector.removeSharedMemory();
                printf("Music Widget: Unrecognized option: %s\n", argv[optind - 1]);
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

    if((ops_mask & OPS_PLAY) == OPS_PLAY) {
        pa_connector.setStateAndExit(MS_PLAYING);
    }

    if((ops_mask & OPS_PAUSE) == OPS_PAUSE) {
        pa_connector.setStateAndExit(MS_PAUSED);
    }

    if((ops_mask & OPS_NEXT) == OPS_NEXT) {
        pa_connector.setStateAndExit(MS_NEXT);
    }

    if((ops_mask & OPS_CLEAR_EXISTING) == OPS_CLEAR_EXISTING)
    {
        song_controller.removeExistingSongs();
        // exit(0);
    }

    if((ops_mask & OPS_FROM_URL) == OPS_FROM_URL)
    {
//         song_controller.fetchSongsFromPlaylist(4, 6);
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
}

int main(int argc, char *argv[])
{
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

    if(online) {
        song_controller.next();
    }
    else if(!pa_connector.openSongToRead(argv[1]))
    {
        pa_connector.removeSharedMemory();
        exit(1);
    }

    pa_connector.initMainloop();

    if(!pa_connector.createContext("Music Widget"))
    {
        std::cerr << "pulse_audio.create_context() failed!";
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
