#!/usr/bin/env bash
#SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
#SPDX-License-Identifier: MIT


EX="$1"

#Note: this calculates trailing twelve month data because the -q flag has been added
#Note: null values will be propagated through all calculations (-l has been removed)
#Note: the -q flag has been removed, so the data will be evaluated using annual data 
cd build
./calculate -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -x "$EX" -d ${EOD_TOOLKIT_HOME}/data/defaultSpreadTable.json -y ${EOD_TOOLKIT_HOME}/data/bondYieldTable.json -w ${EOD_TOOLKIT_HOME}/data/1980_2023_Corporate_Tax_Rates_Around_the_World_Tax_Foundation.csv -c 2.5 -t 0.256 -r 0.025 -e 0.05 -b 1.0 -u 0.2 -n 3 -m 5 -a 35 -o ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/ -q -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/calculate."$EX".log
cd ..
