#!/usr/bin/env bash

EX="$1"
PLOT="$2"
RANK="$3"
cd build
./postprocess -x "$EX" -o ${EOD_TOOLKIT_HOME}/data/"$EX"/postprocess/ -c ${EOD_TOOLKIT_HOME}/data/"$EX"/"$PLOT" -r ${EOD_TOOLKIT_HOME}/data/"$EX"/preprocess/"$RANK" -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -t reportSTU.tex -n -1 -y 2023 -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/report."$EX".log
cd ..

#-m ${EOD_TOOLKIT_HOME}/data/"$EX"/filterByMetricValues.csv -l ${EOD_TOOLKIT_HOME}/data/"$EX"/filterByCountry.csv