#!/usr/bin/env bash

EX="$1"
PLOT="$2"
RANK="$3"
cd build
./postprocess -x "$EX" -o ${EOD_TOOLKIT_HOME}/data/"$EX"/postprocess/ -c ${EOD_TOOLKIT_HOME}/data/"$EX"/"$PLOT" -r ${EOD_TOOLKIT_HOME}/data/"$EX"/preprocess/"$RANK" -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -s ${EOD_TOOLKIT_HOME}/data/"$EX"/filterByTicker_Intel.json -t reportIntel.tex -n -1 -y 2020 -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/postprocessing."$EX".log
cd ..

#-m ${EOD_TOOLKIT_HOME}/data/"$EX"/filterByMetricValues.csv -l ${EOD_TOOLKIT_HOME}/data/"$EX"/filterByCountry.csv
