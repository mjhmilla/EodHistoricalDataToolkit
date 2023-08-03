#!/usr/bin/env bash
cd build
./fetch -f ${EOD_TOOLKIT_HOME}/data/US/historicalData/ -u ${EOD_HISTORICAL_DATA} -x US -k ${EOD_API_TOKEN} -t ${EOD_TOOLKIT_HOME}/data/US/US.json -v
cd ..
#cd build
#./fetch -f ${EOD_TOOLKIT_HOME}/data/historicalData/ -u ${EOD_HISTORICAL_DATA} -x STU -k ${EOD_API_TOKEN} -t ${EOD_TOOLKIT_HOME}/data/STU.json -s 2046 -v
#cd ..
