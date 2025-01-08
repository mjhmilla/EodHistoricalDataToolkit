#!/usr/bin/env bash
#SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
#SPDX-License-Identifier: MIT

./fetchExchanges.sh
./fetchExchangeTickers.sh STU
./fetchFundamentalData.sh STU
#./updateGapsInFundamentalData.sh STU
#./fetchHistoricalData.sh STU
#./updateGapsInHistoricalData.sh STU
#./scanData.sh STU
#./generateDataPatch.sh STU F SWB
#./applyDataPatch.sh STU STU.patch.matching_isin.json

#./calculate.sh STU
#./generateTickerReports.sh STU plot_P_Vp_RcfEv_Roic.csv
#./generateMarketReports.sh STU filterSTU_rank3F.json
