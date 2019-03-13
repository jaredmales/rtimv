## Synopsis

A qt5-based real-time image viewer.

## Motivation

Displays images streaming from a scientific (primarily astronomical) camera.  Such images are expected to be "monochrome", or rather single-color-channel.  rtimv provides convenient ways to change the "stretch", color table, and to perform measurements on the images as they are displayed.  Can handle high frame rates (much higher than monitor refresh).

Originally developed to support the MagAO+VisAO camera.  Now adapted to work with the shared memory image system ImageStreamIO of CACAO.

This is now a stripped down version, with only minimal analysis tools.  A plugin system is under development to enable adding new tools, such as peak fitting.

## Installation

Dependencies:
 - qt-5.
 - milk-org/ImageStreamIO from https://github.com/milk-org/ImageStreamIO

On CentOS-7 you may need to install mesa-libGL-devel.x86_64
 
For `ubuntu` you just need to type `make`.

For `centos 7` you probably need to type `make QMAKE=qmake-qt5`. 

`sudo make install` will install the executable to `/usr/local/bin`.

## User's Guide

To start it: 
```
rtimv shm-name
```
Where `shm-name` comes from the CACAO standard /tmp/shm-name.im.shm

### Zoom level

You can zoom in and out using the mouse wheel (which works opposite to ds9).  Pressing the numbers 1-9 also changes the zoom level.  Arbitrary zoom levels can be set using the control panel.

### Centering

Middle clicking selects that point in the image as the center of the display.  You can press `c` any time to center on the center-pixel of the image.  Arrow keys on the control panel (press `p`) can be used to move the center.

### Color

`rtimv` starts out in a linear min-max color stretch, using the first image it sees to set the min and max value gloabally.  These values do not update image to image.  To re-stretch, that is reseetting the min-max values, you can press `r` to set it pased on the currently displayed image.

You can change the bias and contrast by right-click dragging, just as in ds9. 

An alternative to global min-max is to use the "user box", which can be brought up by pressing `b`, the min and max within this box are then used to set the color stretch.  It can be moved and its size changed to affect which pixels are used to set the stretch.

The control panel (press `p`) can be used to select other stretches (e.g. log) and color tables.

### Stats

A red box for calculating statistics can be brought up with the `r` key.  A circle for measuring radii can be displayed with `o`.  




## Contributors

Creator: Jared R. Males (jaredmales at gmail)

Users: Many improvements have been made based on inputs from users of MagAO+VisAO, especially Kate Follette.

## License

Copyright 2012-2019 Jared R. Males

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
