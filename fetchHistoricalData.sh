#!/usr/bin/env bash

EX="$1"

#rm -r ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/

#mkdir ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/


cd build
./fetch -f ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -u ${EOD_HISTORICAL_DATA} -d ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -x "$EX" -k ${EOD_API_TOKEN} -t ${EOD_TOOLKIT_HOME}/data/"$EX".json -g -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData."$EX".log
cd ..

