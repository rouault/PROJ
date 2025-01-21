#!/bin/bash

set -e

ccache -M 200M

# Avoid using SQLite3 from mono.framework
sudo rm -rf /Library/Frameworks/Mono.framework

CC="clang" CXX="clang++" CFLAGS="-Werror" CXXFLAGS="-Werror" CMAKE_BUILD_TYPE=RelWithDebInfo ./travis/install.sh
