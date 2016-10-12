#!/bin/bash

# Script base directory.
basedir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Dynamic libraries path.
lib_dir=$basedir/lib
[[ "$LD_LIBRARY_PATH" =~ "${lib_dir}" ]] || export LD_LIBRARY_PATH=${lib_dir}:$LD_LIBRARY_PATH
[[ "$LIBRARY_PATH" =~ "${lib_dir}" ]] || export LIBRARY_PATH=${lib_dir}:$LIBRARY_PATH
