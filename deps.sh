#!/bin/bash

JOBS=8
uname -a | grep Darwin >> /dev/null && is_mac=1 || true
uname -a | grep Ubuntu >> /dev/null && is_ubuntu=1 || true

# Parse args
read -r -d '' USAGE << EOM
deps.sh [-x] [-j num_jobs]
        -x enable to print the debug info
        -j number of paralle jobs to use during the build (default: 8)
EOM

[ $# -eq 0 ] && echo "$USAGE" && exit 1

while getopts "xj:" opt; do
    case $opt in
        x)
            SET_X=1;;
        j)
            JOBS=$OPTARG;;
        *)
            echo "$USAGE"
            exit 1;;
        \?)
            echo "Invalid option: -$OPTARG"
            exit 1;;
        :)
            echo "Option $OPTARG requires an argument." >&2
            exit 1
    esac
done

[ -z "$SET_X" ] && echo "SET_X is empty!" >> /dev/null
[ -n "$SET_X" ] && echo "SET_X is not empty!" >> /dev/null && set -x

script_dir=`cd $(dirname $0); pwd`
work_dir=$script_dir
deps_prefix=$work_dir/deps_prefix
deps_src=$work_dir/deps_src

set -eE #enable bash debug mode
error() {
   local sourcefile=$1
   local lineno=$2
   echo abnormal exit at $sourcefile, line $lineno
}
trap 'error "${BASH_SOURCE}" "${LINENO}"' ERR

mkdir -p $deps_prefix

# if [ -n "$FORCE" ]; then
#     rm -rf $deps_prefix
#     rm -rf $deps_build
# fis

function do-cmake() 
{
    cmake -H. -Bcmake-build -DCMAKE_INSTALL_PREFIX:PATH=$deps_prefix \
          -DCMAKE_PREFIX_PATH=$deps_prefix \
          -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release} \
          -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" \
          "$@"
    cmake --build cmake-build -- -j$JOBS
    cmake --build cmake-build -- $make_target install
    rm -rf cmake-build
}

function do-configure () 
{
    ./configure --prefix=$deps_prefix "$@"
}

function do-make() 
{
    make -j$JOBS "$@"
    make install
}

function googletest() 
{
    cd $deps_src/$FUNCNAME
    make_target=all do-cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=1 -DBUILD_GMOCK=OFF
    cd $work_dir
}

googletest
