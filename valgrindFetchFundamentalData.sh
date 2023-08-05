#!/usr/bin/env bash
cd build
./fetch -f ${EOD_TOOLKIT_HOME}/data/STU/fundamentalData/ -u ${EOD_FUNDAMENTAL_DATA} -k ${EOD_API_TOKEN} -t ${EOD_TOOLKIT_HOME}/data/STU.json -x STU -g -v
cd ..
#cd build
#./fetch -f ${EOD_TOOLKIT_HOME}/data/fundamentalData/ -u ${EOD_FUNDAMENTAL_DATA} -k ${EOD_API_TOKEN} -t ${EOD_TOOLKIT_HOME}/data/STU.json -x STU -v
#cd ..
