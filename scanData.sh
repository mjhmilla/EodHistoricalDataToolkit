#!/usr/bin/env bash

EX="$1"

cd build
./scan -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -x "$EX" -o ${EOD_TOOLKIT_HOME}/data/"$EX"/ -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/scan."$EX".log
cd ...


