#!/usr/bin/env bash

EX="$1"
PLOT_SUMMARY="$2"
PLOT_OVERVIEW="$3"
cd build
./generateTickerReports -x "$EX" -o ${EOD_TOOLKIT_HOME}/data/"$EX"/generateTickerReports/ -a ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/ -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -s ${EOD_TOOLKIT_HOME}/data/"$EX"/"$PLOT_SUMMARY" -r ${EOD_TOOLKIT_HOME}/data/"$EX"/"$PLOT_OVERVIEW" -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/generateTickerReports."$EX".log
cd ..
