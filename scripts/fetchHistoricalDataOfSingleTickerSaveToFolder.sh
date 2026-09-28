#!/usr/bin/env bash
#SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
#SPDX-License-Identifier: MIT


EX="$1"
TK="$2"
F="$3"
#rm -r ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/

#mkdir ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/


cd ${EOD_TOOLKIT_HOME}/build
./fetch -f "$F" -i "$TK" -u ${EOD_HISTORICAL_DATA} -d ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -x "$EX" -k ${EOD_API_TOKEN} -t ${EOD_TOOLKIT_HOME}/data/"$EX".json -g -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData."$EX".log
./fetch -f "$F" -i "$TK" -u ${EOD_HISTORICAL_DATA_CSV} -d ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -x "$EX" -k ${EOD_API_TOKEN} -t ${EOD_TOOLKIT_HOME}/data/"$EX".csv -g -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData."$EX".log
cd ..

