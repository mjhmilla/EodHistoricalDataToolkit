#!/usr/bin/env bash
EX="$1"
./fetchExchanges.sh 
./fetchTickers.sh "$EX"
./updateGapsInFundamentalData.sh "$EX"
./updateGapsInHistoricalData.sh "$EX"
