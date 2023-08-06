#!/usr/bin/env bash
EX="$1"


#rm -r ${EOD_TOOLKIT_HOME}/data/"$EX"/analysisData/

#mkdir ${EOD_TOOLKIT_HOME}/data/"$EX"/analysisData/

./fetchExchanges.sh 
./fetchExchangeTickers.sh "$EX"
./fetchFundamentalData.sh "$EX"

