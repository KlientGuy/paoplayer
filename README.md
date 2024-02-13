# Pulse audio online player
That's a stupid name I know

### Chapters
* [Introduction](#introduction)
* [Why did I write this](#why)
* [Should you use it](#should)
* [Available options](#options)
* [Configuration file](#config)
* [About contributing](#contrib)
* [Build it yourself](#build)
* [Technical detail](#tech)
* [Personal thoughts](#personal)

---

<a name="introduction"></a>
### Introduction (what even is this)
This is a simple music player written in c++ utilizing pulseaudio sinks as it's sound interface. \
It makes use of [yt-dlp](https://github.com/yt-dlp/yt-dlp) (huge thanks to all the contributors :heart:, as that's what really allowed me to create this project) to heavy lift the "_online_" part.

<a name="why"></a>
### Why did I write this?
Because a firefox window that exists solely for the purpose of playing music is a bit overkill (yes I refused to use existing software, because how hard it is to write your own music player). \
The other reason is that I wanted to explore c++ some more and also wondered how can you play sounds with it.

<a name="should"></a>
### Should you use it
I mean if you really want to then it's working.

Currently you can:
* Play music by passing a file path
* Play a single track from ex. Youtube by passing a video url
* Play a playlist by passing a url (I mostly use that one)
* Pause and resume
* Skip to the next track
* Change volume

---

### How to install
Make sure you have [yt-dlp](https://github.com/yt-dlp/yt-dlp) installed, then
just go to the releases page and download the latest binary

---

<a name="options"></a>
### Available options:

<font size="4"> **Init:** </font>
---
```
    -U  --from-url <track or playlist URL>      Play from that URL
    -c  --clear-existing                        Clear a songs directory
    --single                                    Play a single song instead of a playlist.
                                                You then have to pass video url to -U
```
<font size="4"> **Track control:** </font>
```
    -p  --pause                                 Pause currently playing track
    -P  --play                                  Resume current track
    -n  --next                                  Next track
    -u  --volume-up <float>                      Increase the volume by amount
    -d  --volume-down <float>                    Decrease the volume by amount
```
<font size="4"> **Extra:** </font>
```
    -g  --currently-playing                     Print currently playing track

    -q  --quiet                                 Supress most output
    -v  --verbose                               Extra output
    -D  --debug                                 Debug mode (you probably shouldn't use that)
```

---

<a name="config"></a>
### There is also a small configuration file
Your config path should be `~/.config/paoplayer/config`

<font size="4"> **Config options:** </font>
```cpp
    default_playlist_url = string //If no url is passed to -U it uses this instead

    //Amout of songs to download in advance if current is the last downloaded
    preload_count = int 
    save_previous = bool //If false, delete previous songs (excluding previous)
    song_dir = string //Where to store downloaded songs
    browser_cookies = string //Browser to use cookies from for yt-dlp
```

<font size="4"> **Example configuration** </font>
```
    default_playlist_url = https://www.youtube.com/playlist?list=PLOstxpSiJGlClyRnCiubdXJcUvwgHcrVH
    preload_count = 3
    save_previous = false
    song_dir = /tmp/music_widget/
    browser_cookies = firefox
```

---

<a name="contrib"></a>
### About contributing
If you're bored enought that you want to contribute to this piece of software, issues and discussions are welcome, but I probably wont accept PR's.

I treat this project as a mean to expand my knowledge and I want it to stay that way.

As I said if you want to see a feature being added or a bug being fixed go ahead and create an issue. I'll probably look at that when I'll have the time to do so.

---

<a name="build"></a>
### Build it yourself
I've used ninja because I didn't know any better and Clion does that by default (been moving to neovim lately)

Run these commands in the root of the project
```
cmake -S ./music_widget_cli -B ./music_widget_cli/cmake-build-debug/ -Ddebug=false`
```
```
ninja -C ./music_widget_cli/cmake-build-debug/
```

---

<a name="tech"></a>
### Technical detail
I've decided to use shared memory (or rather memory mapped file) to allow changing state by running another proccess of the app, so sometimes if things go really wrong that file is not removed by the program. If you run it once again and it exits immediately that probably means that it cleaned up that file and everything should be ok :)

Don't ask about reasoning, I've just wanted to use that :stuck_out_tongue:

---

<a name="personal"></a>
### Personal thoughts
I mainly use it for youtube (music) to play it's auto mix and I have that set as my default url. It works every time and I'm happy with its ~8 mb RAM usage so don't expect anything ultra performant.

Also I'm **not** a professional c++ programmer (I do php for a living), so the code is probably shitty, but it suits my needs so who cares.

Currenlty I only support Linux as that's my primary OS and don't plan to build a windows version anytime soon.
