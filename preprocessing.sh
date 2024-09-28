#!/usr/bin/env bash

EX="$1"
ALGO="$2"
RANK="$3"
cd build
./preprocess -x "$EX" -o ${EOD_TOOLKIT_HOME}/data/"$EX"/preprocess/ -c ${EOD_TOOLKIT_HOME}/data/"$EX"/"$ALGO" -r ${EOD_TOOLKIT_HOME}/data/"$EX"/rankData/"$RANK" -a ${EOD_TOOLKIT_HOME}/data/"$EX"/analysisData/ -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -n 2 -q -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/preprocessing."$EX".log
cd ..
