#!/usr/bin/env bash
#SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
#SPDX-License-Identifier: MIT

EX="$1"


#rm -r ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/

#mkdir ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/

./fetchExchanges.sh 
./fetchExchangeTickers.sh "$EX"
./fetchHistoricalData.sh "$EX"
