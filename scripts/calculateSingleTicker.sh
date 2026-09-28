#!/usr/bin/env bash
#SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
#SPDX-License-Identifier: MIT


EX="$1"
HC="$2"
TK="$3"
YA="$4"
#Note: this calculates trailing twelve month data because the -q flag has been added
#Note: null values will be propagated through all calculations (-l has been removed)
#Note: the -q flag has been removed, so the data will be evaluated using annual data 
cd ${EOD_TOOLKIT_HOME}/build
./calculate -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -r ${EOD_TOOLKIT_HOME}/data/forex/ -c ${EOD_TOOLKIT_HOME}/config/calculate-config.json -x "$EX" -i "$TK" -m "$HC" -n "$YA" -o ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/ -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/calculate."$EX".log
cd ..
