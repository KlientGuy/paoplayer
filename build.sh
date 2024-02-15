buildMode=${1}

if [ -z "$buildMode" ]; then
    echo -e "\x1b[38;2;255;0;0mYou have to specify build mode\x1b[0m"
    exit 1;
fi

if [ "$buildMode" = 'release' ]; then
    cmake -S ./music_widget_cli/ -B ./music_widget_cli/cmake-build-release -Ddebug=false -DCMAKE_BUILD_TYPE=Release
    cmake --build ./music_widget_cli/cmake-build-release/
elif [ "$buildMode" = 'debug' ]; then
    cmake -S ./music_widget_cli/ -B ./music_widget_cli/cmake-build-debug -Ddebug=true -DCMAKE_BUILD_TYPE=Debug
    ninja -C $outdir
else
    echo -e "\x1b[38;2;255;0;0m\"$1\" Is not a valid build mode (valid modes are release and debug)\x1b[0m"
    exit 1;
fi


