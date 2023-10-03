#!/usr/bin/env bash

EX="$1"
HEX="$2"
TVE="$3"

cd build
./generatePatch -c ${EOD_TOOLKIT_HOME}/data/"$EX"/"$EX".scan.json -x "$EX" -s ${EOD_TOOLKIT_HOME}/data/exchange-symbol-list/ -l ${EOD_TOOLKIT_HOME}/data/exchanges.json -o ${EOD_TOOLKIT_HOME}/data/"$EX"/ -m ${EOD_TOOLKIT_HOME}/data/exchange-symbol-list/"$HEX".json -t "$TVE" -v| tee ${EOD_TOOLKIT_HOME}/data/"$EX"/generatePatch."$EX".log
cd ..

