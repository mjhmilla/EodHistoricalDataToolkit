#!/usr/bin/env bash

EX="$1"

cd build
./calculate -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -x STU -d ${EOD_TOOLKIT_HOME}/data/defaultSpreadTable.json -y ${EOD_TOOLKIT_HOME}/data/bondYieldTable.json -c 2.5 -t 0.40 -r 0.025 -e 0.05 -b 1.0 -u 0.2 -n 3 -m 5 -a 35 -l -o ${EOD_TOOLKIT_HOME}/data/"$EX"/analysisData/ -v
cd ..
