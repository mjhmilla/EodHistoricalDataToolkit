#!/usr/bin/env bash
EX="$1"

rm -r ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/
rm -r ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/
rm -r ${EOD_TOOLKIT_HOME}/data/"$EX"/analysisData/

mkdir ${EOD_TOOLKIT_HOME}/data/"$EX"/fundamentalData/
mkdir ${EOD_TOOLKIT_HOME}/data/"$EX"/historicalData/
mkdir ${EOD_TOOLKIT_HOME}/data/"$EX"/analysisData/

./fetchExchanges.sh 
./fetchTickers.sh "$EX"
./fetchFundamentalData.sh "$EX"
./fetchHistoricalData.sh "$EX"
