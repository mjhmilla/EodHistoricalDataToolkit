#!/usr/bin/env bash
EX="$1"

#./fetchExchanges.sh 
#./fetchExchangeTickers.sh "$EX"
#./fetchFundamentalData.sh "$EX"

./updateGapsInFundamentalData.sh "$EX"
./fetchHistoricalData.sh "$EX"
./scanData "$EX"
