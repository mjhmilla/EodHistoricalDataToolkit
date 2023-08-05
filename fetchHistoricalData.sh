#!/usr/bin/env bash
EX="$1"
cd build
./fetch -f ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -u ${EOD_HISTORICAL_DATA} -x "$EX" -k ${EOD_API_TOKEN} -t ${EOD_TOOLKIT_HOME}/data/"$EX"/"$EX".json -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData."$EX".log
cd ..

