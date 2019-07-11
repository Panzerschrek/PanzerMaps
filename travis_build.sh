#!/bin/bash

# Configure build
mkdir build-travis &&\
cd build-travis &&\
cmake ../source/ -DCMAKE_BUILD_TYPE=Release &&\
\
# Build it
# travis-ci has 2 cpu cores
make -j 2