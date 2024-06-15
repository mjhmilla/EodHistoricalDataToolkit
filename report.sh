#!/usr/bin/env bash

EX="$1"
ALGO="rankByValueRCashFlowExcessRoic.csv"
RANK="rankByValueRCashFlowExcessRoic_ranking.json"
cd build
./report -x "$EX" -o ${EOD_TOOLKIT_HOME}/data/"$EX"/report/ -c ${EOD_TOOLKIT_HOME}/data/"$EX"/"$ALGO" -r ${EOD_TOOLKIT_HOME}/data/"$EX"/rankData/"$RANK" -a ${EOD_TOOLKIT_HOME}/data/"$EX"/analysisData/ -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -n 10 -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/report."$EX".log
cd ..
