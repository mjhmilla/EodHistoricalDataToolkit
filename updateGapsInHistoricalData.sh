#!/usr/bin/env bash
EX="$1"
cd build
./fetch -f ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -d ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -u ${EOD_HISTORICAL_DATA} -x "$EX" -k ${EOD_API_TOKEN} -t ${EOD_TOOLKIT_HOME}/data/"$EX".json -g -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/updateGapsHistoricalData."$EX".log
cd ..

