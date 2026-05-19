#!/usr/bin/env bash
#SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
#SPDX-License-Identifier: MIT

EX="$1"

#./fetchExchanges.sh 
#./fetchExchangeTickers.sh "$EX"
#./fetchFundamentalData.sh "$EX"

./updateGapsInFundamentalData.sh "$EX"
./fetchHistoricalData.sh "$EX"
./scanData "$EX"
