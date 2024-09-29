#!/usr/bin/env bash

EX="$1"

#Note: this calculates trailing twelve month data because the -q flag has been added
cd build
./calculate -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -x "$EX" -d ${EOD_TOOLKIT_HOME}/data/defaultSpreadTable.json -y ${EOD_TOOLKIT_HOME}/data/bondYieldTable.json -w ${EOD_TOOLKIT_HOME}/data/1980_2023_Corporate_Tax_Rates_Around_the_World_Tax_Foundation.csv -c 2.5 -t 0.256 -r 0.025 -e 0.05 -b 1.0 -u 0.2 -n 3 -m 5 -a 35 -l -o ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/ -q -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/calculate."$EX".log
cd ..
