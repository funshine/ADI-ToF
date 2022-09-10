#!/bin/bash

# This script builds the sdk.

build() {
        source_dir=$(cd "$(dirname "$0")/../.."; pwd)

        if [[ -z "${build_dir}" ]]; then
                build_dir=${source_dir}/build
        fi

        echo "The sdk will be built in: ${build_dir}"

        mkdir -p "${build_dir}"

        deps_install_dir="/opt"
        CMAKE_OPTIONS="-DNXP=1 -DWITH_OFFLINE=on -DUSE_DEPTH_COMPUTE_STUBS=on"
        PREFIX_PATH="${deps_install_dir}/glog;${deps_install_dir}/protobuf;${deps_install_dir}/websockets;"

        pushd "${build_dir}"
        cmake "${source_dir}" ${CMAKE_OPTIONS} -DCMAKE_PREFIX_PATH="${PREFIX_PATH}"
        make -j ${NUM_JOBS}
}

build $@
