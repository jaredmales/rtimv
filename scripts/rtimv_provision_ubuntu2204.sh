#!/bin/bash
set -o pipefail
# since we care about almost every exit status, check them all. use `|| true` to bypass.
set -e
#########################################################################
# A script to provision an Ubuntu 22.04 machine to run rtimv
#
#
#
# Creates a directory called ~Source TODO: make this configurable
# Switches some repos to dev TODO: make this configurable
#########################################################################

# from https://github.com/milk-org/milk/blob/dev/Dockerfile
sudo apt-get install -y \
    git \
    make \
    dpkg-dev \
    libc6-dev \
    cmake \
    pkg-config \
    python3-dev \
    libcfitsio-dev \
    pybind11-dev \
    python3-pybind11 \
    libgsl-dev \
    libfftw3-dev \
    libncurses-dev \
    libbison-dev \
    libfl-dev \
    libreadline-dev \
    gfortran libopenblas-dev liblapacke-dev \
    pkg-config \
    gcc \
    g++ \

# for this script
sudo apt-get install -y wget

## Make work directory
mkdir -p ~/Source
cd ~/Source

if ! command -v milk; then
    if [[ ! -d milk ]]; then
        git clone --recursive https://github.com/milk-org/milk.git
    fi
    cd milk
    git checkout dev
    mkdir -p _build
    cd _build
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
    sudo make install
    if [[ ! -h /usr/local/milk ]]; then
        ln -sfv /usr/local/milk-* /usr/local/milk
    fi
fi

## Setup milk for linking TODO: check if already done
echo /usr/local/milk/lib/ | sudo tee -a  /etc/ld.so.conf.d/milk.conf
sudo ldconfig

## xrif
if [[ ! -d xrif ]]; then
    git clone https://github.com/jaredmales/xrif.git
fi
mkdir -p ./xrif/_build
cd ./xrif/_build
cmake ..
make
sudo make install

## zeromq drafts API
cd ~/Source
if [[ ! -d zeromq-4.3.4 ]]; then
    wget https://github.com/zeromq/libzmq/releases/download/v4.3.4/zeromq-4.3.4.tar.gz
    tar -xvzf zeromq-4.3.4.tar.gz
fi
cd zeromq-4.3.4
./configure --enable-drafts
make
sudo make install

cd ~/Source
if [[ ! -d cppzmq ]]; then
    git clone https://github.com/zeromq/cppzmq.git
fi
cd cppzmq/
sudo cp *.hpp /usr/local/include/

## milkzmq
cd ~/Source
if [[ ! -d milkzmq ]]; then
    git clone https://github.com/jaredmales/milkzmq.git
fi
cd milkzmq
git checkout dev
make
sudo make install

## mxlib
cd ~/Source
sudo apt-get install -y libgsl-dev libboost-all-dev libcfitsio-dev libopenblas-dev libfftw3-dev libeigen3-dev
if [[ ! -d mxlib ]]; then
    git clone https://github.com/jaredmales/mxlib.git
fi
cd mxlib
git checkout dev
echo NEED_CUDA=no > local/Common.mk
echo PREFIX=/usr/local >> local/Common.mk
make
sudo make install

## rtimv
cd ~/Source
sudo apt install -y qtbase5-dev qt5-qmake

if [[ ! -z $GITHUB_WORKSPACE ]]; then
    # Only if we're running in GitHub Actions
    cd $GITHUB_WORKSPACE
elif [[ ! -d rtimv ]]; then
    # Only if ~/Source/rtimv doesn't exist
    git clone https://github.com/jaredmales/rtimv.git
    cd rtimv
    git checkout dev
else
    cd rtimv
    git checkout dev
fi
make
sudo make install

