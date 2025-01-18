#!/usr/bin/env bash
#SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
#SPDX-License-Identifier: MIT


EX="$1"
CONFIG="$2"
cd build
./generateScreenerReport -x "$EX" -t ${EOD_TOOLKIT_HOME}/data/"$EX"/generateTickerReports/ -o ${EOD_TOOLKIT_HOME}/data/"$EX"/generateScreenerReport/ -a ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/ -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -c ${EOD_TOOLKIT_HOME}/data/"$EX"/"$CONFIG" -v | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/generateScreenerReport."$EX".log
cd ..

