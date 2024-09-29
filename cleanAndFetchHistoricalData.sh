#!/usr/bin/env bash
EX="$1"


#rm -r ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/

#mkdir ${EOD_TOOLKIT_HOME}/data/"$EX"/calculateData/

./fetchExchanges.sh 
./fetchExchangeTickers.sh "$EX"
./fetchHistoricalData.sh "$EX"
