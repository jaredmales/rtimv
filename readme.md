## Synopsis

A qt5-based real-time image viewer.

## Motivation

Displays images streaming from a scientific (primarily astronomical) camera.  Such images are expected to be "monochrome", or rather single-color-channel.  rtimv provides convenient ways to change the "stretch", color table, and to perform measurements on the images as they are displayed.  Can handle high frame rates (much higher than monitor refresh).

Originally developed to support the MagAO+VisAO camera.  Now adapted to work with the shared memory image system ImageStreamIO of CACAO.

This is now a stripped down version, with only minimal analysis tools.  A plugin system enables adding new tools, such as peak fitting.

## Installation

Dependencies:
 - qt-5
   - on Ubuntu 20 `sudo apt-get install qt5-default`
   - on Ubuntu 22 `sudo apt install qtbase5-dev qt5-qmake`
 - mxlib [https://github.com/jaredmales/mxlib](https://github.com/jaredmales/mxlib)
   - for best results checkout the `magaox` branch of mxlib
 - milk-org/milk  from https://github.com/milk-org/milk
   - If you are looking for an image viewer for cacao, you probably already have this!
   - If not follow the instructions in the README to checkout milk and build it.
   - See also https://github.com/milk-org/milk/wiki/install-DownloadCompile, I recommend creating the `tmpfs` for `milk/shm`
 - milkzmq from https://github.com/jaredmales/milkzmq


On CentOS-7 you may need to install mesa-libGL-devel.x86_64
 
For `ubuntu` you just need to type `make`.  This should work on `CentOS 7` as well, but if not try calling it with `make QMAKE=qmake-qt5`. 

Once make completes, `sudo make install` will install the executable to `/usr/local/bin`.  You may need to run `sudo ldconfig` to update the linker.

## User's Guide

A user's guide is located in [doc/UserGuide.md](doc/UserGuide.md).


## Contributors

Creator: Jared R. Males (jaredmales at gmail)

Users: Many improvements have been made based on inputs from users of MagAO+VisAO, especially Kate Follette.

## License

Copyright 2012-2022 Jared R. Males

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
