#!/bin/bash


export CC=${HOME}/usr/llvm/bin/clang
export CXX=${HOME}/usr/llvm/bin/clang++

export LIBBASE_PATH=${HOME}/usr/xbase-libcxx


cmake -B cmake-build-release -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=${CXX} \
    -DCMAKE_C_COMPILER=${CC} \
    -DCMAKE_CXX_FLAGS="-g -o0 -std=c++23 -stdlib=libc++" \
    -DCMAKE_CXX_STANDARD=23 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_PREFIX_PATH=${LIBBASE_PATH}/lib/cmake \
    -DLIBBASE_PATH=${LIBBASE_PATH}

ninja -C cmake-build-release 