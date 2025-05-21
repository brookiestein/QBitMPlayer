# QBitMPlayer
Music Player written in C++ &amp; Qt.

## Installation
### Binaries
QBitMPlayer is currently available to be installed in its binary form just for [Windows](https://github.com/brookiestein/QBitMPlayer/releases). Packages will be provided for, at least, the most popular Linux distributions as well.

### Installing From Source
#### Dependencies
1. QtWidgets
2. QtMultimedia
3. QtDBus (if you want support for Inter-Proccess Comunication)
4. CMake (for building)

Make sure you have all those packages installed on your OS. Installation will depend on what Operating System you're running, 
but here's a table showing how to install them in the most popular Linux distributions:

|Distribution | Command                                                                        |
|-------------|--------------------------------------------------------------------------------|
|Debian/Ubuntu| apt install libqt6widgets5-dev libqt6dbus6 libqt6multimedia6                   |
|Fedora       | dnf install qt6-qtmultimedia{,-devel} qt6-widgets{,-devel} qt6-qtbase{,-devel} |
|Arch Linux   | pacman -S qt6-multimedia qt6-base qt6-widgets                                  |
|Gentoo       | emerge -a qtmultimedia:6 qtbase:6 (enable the dbus, and widgets USE flags)     |

*Please make sure to execute the corresponding command with super user rights.*

*The Qt Maintenance Tool can also be used to install all the dependencies.*

**If you're using Gentoo Linux, QBitMPlayer is available in the [GURU](https://github.com/gentoo/guru) repository, just run** `emerge -a media-sound/qbitmplayer`.

Once the dependencies are installed, proceed to the building:
1. First, get the sources:
```
$ git clone https://github.com/brookiestein/QBitMPlayer
```
2. Build
```
$ cd QBitMPlayer
$ cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/usr -DUSE_IPC=1
$ cmake --build build --parallel
```
In the second command, the `-DUSE_IPC=1` macro tells the project you want to have support for Inter-Proccess Communication, omit that if you don't want it.

3. Install
```
# cmake --build build --target install
```
**Please note this last command needs to be executed as super user as QBitMPlayer will be installed in /usr which belongs to the root user.**

QBitMPlayer will be installed in `/usr/bin/qbitmplayer`.

This is a table showing the available command line options:

|Option            | Description
|------------------|---------------------------------------------------------------------------------------|
|-f, --files       | Comma-separated list of music files to be played.                                     |
|-l, --language    | Which language to display the app in other than English. Currently available: Spanish |
|-n, --next        | Tell an existing QBitMPlayer instance to play the next song if any.                   |
|-P, --previous    | Tell an existing QBitMPlayer instance to play the previous song if any.               |
|-t, --toggle-play | Tell an existing QBitMPlayer instance to resume or pause the player.                  |
|-s, --stop        | Tell an existing QBitMPlayer instance to stop the player.                             |

Notice that the last four options are only available when built with IPC support.

QBitMPlayer is free and open source, you can use it and modify it freely!

If you feel you want to thank me for my work, you can [buy me a coffee](https://buymeacoffee.com/brayan0x1e).
