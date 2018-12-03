## Synopsis

A qt-based real-time image viewer.

## Motivation

Displays images streaming from a scientific (primarily astronomical) camera.  Such images are expected to be "monochrome", or rather single-color-channel.  rtimv provides convenient ways to change the "stretch", color table, and to perform measurements on the images as they are displayed.  Can handle high frame rates (much higher than monitor refresh).

Originally developed to support the MagAO+VisAO camera.  Now adapted to work with the shared memory image system developed for SCExAO, and now used in MILK/CACAO.

## Installation

Dependencies:
 - qt-5.
 - qwt.
 - levmar. See my instructions for this at https://jaredmales.github.io/mxlib/group__levmar.html
 - BLAS & Lapack. See my instructions for this at https://jaredmales.github.io/mxlib/group__mkl.html
 - milk-org/ImageStreamIO from https://github.com/milk-org/ImageStreamIO

After installing the dependencies, verify, and edit if necessary, the following variables in `Makefile`
 - QMAKE
 - CACAO_INCLUDE
 - QWT_INCLUDE
 - QWT_LIB
 - LEVMAR_INC
 - LEVMAR_LIB
 - BLASPACK

Now all you should have to do is type make in the top directory.

## User's Guide

To start it: ./bin/rtimv /tmp/path_to_shm.im.shm

I have not updated the user guide since porting from VisAO.  You can see [the old one here,](https://visao.as.arizona.edu/software_files/visao/html/group__operators__users__guide.html#imviewer_userguide) which is still mostly correct.  In particular the keyboard shortcuts should all work.


## Contributors

Creator: Jared R. Males (jaredmales at gmail)

Users: Many improvements have been made based on inputs from users of MagAO+VisAO, especially Kate Follette.

## License

Copyright 2012-2017 Jared R. Males

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
