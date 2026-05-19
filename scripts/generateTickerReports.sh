#!/usr/bin/env bash
#SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
#SPDX-License-Identifier: MIT


EX="$1"
PLOT_CONFIG="$2"
cd build
./generateTickerReports -x "$EX" -o ${EOD_TOOLKIT_HOME}/data/"$EX"/generateTickerReports/ -a ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/ -f ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/ -p ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/ -c ${EOD_TOOLKIT_HOME}/config/"$PLOT_CONFIG" -v -g | tee ${EOD_TOOLKIT_HOME}/data/"$EX"/generateTickerReports."$EX".log
cd ..
