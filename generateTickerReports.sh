#!/usr/bin/env bash

EX="$1"
PLOT="$2"
cd build
./generateTickerReports -x "$EX" -o ${EOD_TOOLKIT_HOME}/data/"$EX"/generateTickerReports/ -a ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/ -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -c ${EOD_TOOLKIT_HOME}/data/"$EX"/"$PLOT" -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/generateTickerReports."$EX".log
cd ..
