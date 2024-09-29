#!/usr/bin/env bash

EX="$1"
ALGO="$2"
cd build
./rank -x "$EX" -i ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/ -o ${EOD_TOOLKIT_HOME}/data/"$EX"/rankData/ -r ${EOD_TOOLKIT_HOME}/data/"$EX"/"$ALGO" -q -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/rank."$EX".log
cd ..
