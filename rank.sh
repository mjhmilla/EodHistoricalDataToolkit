#!/usr/bin/env bash

EX="$1"
ALGO="$2"
cd build
./rank -x "$EX" -i ${EOD_TOOLKIT_HOME}/data/"$EX"/analysisData/ -o ${EOD_TOOLKIT_HOME}/data/"$EX"/rankData/ -r ${EOD_TOOLKIT_HOME}/data/"$EX"/"$ALGO" -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -n -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/rank."$EX".log
cd ..
