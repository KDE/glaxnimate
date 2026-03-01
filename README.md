Glaxnimate
=======================================

About
---------------------------------------

A simple vector graphics animation program.


Dependencies
---------------------------------------

* C++17 compliant compiler
* Qt6 >= 6.2
* CMake >= 3.5
* Python3
* Potrace
* libav (libavformat, libswscale, libavcodec, libavutil) >= 59
* libarchive
* KF6


Getting the Latest Code
---------------------------------------

You can find the code on [GitLab](https://gitlab.com/mattbas/glaxnimate).

To clone with git:

    git clone --recursive https://gitlab.com/mattbas/glaxnimate.git


Building
---------------------------------------

### Generic Overview

If you are building from git, ensure your submodules are up to date

    git submodule update --init --recursive

Standard CMake build commands work

    mkdir build
    cd build
    cmake ..
    make -j 4 # This uses 4 cores to compile

It will produce the executable `bin/glaxnimate` relative to the build directory


### Deb-based systems (Ubuntu, Debian, etc)

Install the dependencies:

    apt-get install -y g++ cmake extra-cmake-modules qt6-base-dev qt6-tools-dev qt6-svg-dev \
    qt6-image-formats-plugins libpython3-dev libpotrace-dev \
    libarchive-dev libavformat-dev libswscale-dev libavcodec-dev libavutil-dev \
    libkf6coreaddons-dev libkf6crash-dev libkf6widgetsaddons-dev libkf6xmlgui-dev \
    libkf6archive-dev libkf6completion-dev

The generic `cmake` commands listed above should work.

To use Qt6 install these instead:

    apt install -y g++ cmake extra-cmake-modules \
    qt6-base-dev qt6-tools-dev qt6-svg-dev qt6-image-formats-plugins \
    libpython3-dev libpotrace-dev \
    libarchive-dev libavformat-dev libswscale-dev libavcodec-dev libavutil-dev \
    libkf6coreaddons-dev libkf6crash-dev libkf6widgetsaddons-dev libkf6xmlgui-dev \
    libkf6archive-dev libkf6completion-dev libkf6iconthemes-dev

Then enable Qt6 with cmake:

    cmake .. -DQT_VERSION_MAJOR=6


### Arch-based systems

Install the dependencies:

    pacman -S git base-devel cmake extra-cmake-modules python zlib hicolor-icon-theme \
    potrace ffmpeg qt6-base qt6-tools qt6-imageformats qt6-svg

The generic `cmake` commands listed above should work.


### MacOS

Install the dependencies with homebrew:

    brew install cmake extra-cmake-modules qt@6 python potrace ffmpeg libarchive 
    # for kf6 (for now) see: https://github.com/KDE/homebrew-kde

    brew untap kde-mac/kde 2> /dev/null
    brew tap kde-mac/kde https://invent.kde.org/packaging/homebrew-kde.git --force-auto-update
    "$(brew --repo kde-mac/kde)/tools/do-caveats.sh"

    brew tap homebrew/core --force
    brew tap-new $USER/local-kde-mac
    brew extract --version=6.22.0 ki18n $USER/homebrew-local-kde-mac
    brew extract --version=6.22.0 karchive $USER/homebrew-local-kde-mac

    brew install --HEAD $USER/local-kde-mac/ki18n@6.22.0 
    brew install --HEAD $USER/local-kde-mac/karchive@6.22.0
    
    brew install kde-mac/kde/kf6-kcoreaddons kde-mac/kde/kf6-kwidgetsaddons kde-mac/kde/kf6-kxmlgui kde-mac/kde/kf6-kcompletion kde-mac/kde/kf6-kiconthemes kde-mac/kde/kf6-kconfig kde-mac/kde/kf6-kcrash
    $(brew --repo kde-mac/kde)/tools/do-caveats.sh

Build with `cmake`, specifying the Qt installation path:

    mkdir build
    cd build
    cmake .. -DKF_MAJOR=6 -DQT_MAJOR_VERSION=6 -DQT_VERSION_MAJOR=6 -DQt6_DIR="$(brew --prefix qt@6)/lib/cmake/Qt6" -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)/lib/cmake/Qt6Designer"
    make

To simplify the build process, you can also use the provided scripts (currently not completely working with homebrew Qt5 / Apple Silicon, so prefer the above method):

    ./deploy/mac_build.sh deps
    ./deploy/mac_build.sh configure
    ./deploy/mac_build.sh build


### Windows

Install [MSYS2](https://www.msys2.org/), select "Mingw-w64 64 bit".

To simplify the build process, you can use the provided scripts:

    ./deploy/win_build.sh deps
    ./deploy/win_build.sh configure
    ./deploy/win_build.sh build
    

### Android

See https://develop.kde.org/docs/packaging/android/

#### Running

There are some utility `make` targets to run the apk

    # List available virtual devices (you need to create them separately)
    make android_avd_list
    # Start virtual device ("device" is from `make android_avd_list`)
    make android_avd_start DEVICE="device"

    # Build the APK
    make glaxnimate_apk -j8
    # Install and run the APK on the running AVD
    make android_install
    # Attach to the output
    make android_log

Once you have an AVD running (or a debug phone connected) you can run a single line:

    make glaxnimate_apk -j8 && make android_install && make android_log

#### Troubleshooting

**Could not determine java version**

Qt downloads an old version of `gradle`, so if you get an error that your
Java version cannot be recognize find the file `gradle-wrapper.properties`
and update it to version 5.1 or later.

**Incompatible target / Undefined reference / Redefinition**

Sometimes `qmake` messes up the build, the best option is to remove the build
directory created by Qt Creator and rebuild.

**Invalid MinSdkVersion**

Depending on which version of the Android SDK you have, you might have to select 
a different value in `src/android/android/AndroidManifest.xml`.


**libc++.so not found**

The Android toolkit stuff has some wrong paths, it can be fixed like this:

    sudo mkdir /opt/android 
    sudo ln -s $HOME/Android/Sdk/ndk/21.1.6352462/ /opt/android/android-ndk-r19c
    
Where `$HOME/Android/Sdk/ndk/21.1.6352462/` is the directory that Qt creator 
used to install the Android NDK, and `/opt/android/android-ndk-r19c` is the 
directory mentioned in the error message.

**Cannot set up Android, not building an APK / Application binary is not in output directory**

Sometimes it does that, building again usually fixes it.


### Installation / Packaging

Assuming you configured / compiled for your target system, to install
to a specific directory, run the following commands:

```bash
# Ensure the translations files are compiled
make translations
# Install under /installation/path
make install DESTDIR=/installation/path
```

If you want to just install on your system, you don't need to specify `DESTDIR`.


Contacts
---------------------------------------

* [Telegram (Chat)](https://t.me/Glaxnimate)
* [KDE GitLab (Code)](https://invent.kde.org/graphics/glaxnimate)
* [Bugtracker](https://bugs.kde.org/enter_bug.cgi?product=glaxnimate).
* [User Manual](https://docs.glaxnimate.org)


License
---------------------------------------

GPLv3 or later, see COPYING.

Copyright (C) 2020 Mattia Basaglia

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
