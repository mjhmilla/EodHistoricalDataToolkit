#!/usr/bin/env bash

EX="$1"
CONFIG="$2"
cd build
./generateMarketReports -x "$EX" -t ${EOD_TOOLKIT_HOME}/data/"$EX"/generateTickerReports/ -o ${EOD_TOOLKIT_HOME}/data/"$EX"/generateMarketReports/ -a ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/ -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -c ${EOD_TOOLKIT_HOME}/data/"$EX"/"$CONFIG" -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/generateMarketReports."$EX".log
cd ..

