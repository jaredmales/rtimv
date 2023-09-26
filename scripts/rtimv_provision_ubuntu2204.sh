#!/bin/bash
set -o pipefail

#########################################################################
# A script to provision an Ubuntu 22.04 machine to run rtimv
#
# Assumes milk is already installed and symlinked from /usr/local/milk
#
#
# Creates a directory called ~Source TODO: make this configurable
# Switches some repos to dev TODO: make this configurable
#########################################################################

## Setup milk for linking TODO: check if already done
echo /usr/local/milk/lib/ | sudo tee -a  /etc/ld.so.conf.d/milk.conf
sudo ldconfig

## Make work director
mkdir -p ~/Source
cd ~/Source

## xrif
git clone https://github.com/jaredmales/xrif.git
mkdir -p ./xrif/_build
cd ./xrif/_build
cmake ..
make
sudo make install

## zeromq drafts API
cd ~/Source
wget https://github.com/zeromq/libzmq/releases/download/v4.3.4/zeromq-4.3.4.tar.gz
tar -xvzf zeromq-4.3.4.tar.gz
cd zeromq-4.3.4
./configure --enable-drafts
make
sudo make install

cd ~/Source
git clone https://github.com/zeromq/cppzmq.git
cd cppzmq/
sudo cp *.hpp /usr/local/include/

## milkzmq
cd ~/Source
git clone https://github.com/jaredmales/milkzmq.git
cd milkzmq
git checkout dev
make
sudo make install

## mxlib
cd ~/Source
sudo apt install -y libgsl-dev libboost-all-dev libcfitsio-dev libopenblas-dev libfftw3-dev libeigen3-dev
git clone https://github.com/jaredmales/mxlib.git
cd mxlib
git checkout dev
echo NEED_CUDA=no >> local/Common.mk
echo PREFIX=/usr/local >> local/Common.mk
make
sudo make install

## rtimv
cd ~/Source
sudo apt install -y qtbase5-dev qt5-qmake
git clone https://github.com/jaredmales/rtimv.git
cd rtimv
git checkout dev
make
sudo make install

