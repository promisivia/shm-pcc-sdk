#!/bin/sh
cd papi-5.7/src
./configure
make -j && sudo make install
