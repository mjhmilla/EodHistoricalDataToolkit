#!/usr/bin/env bash
#SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
#SPDX-License-Identifier: MIT

#./fetchExchanges.sh
#./fetchExchangeTickers.sh F
#./fetchFundamentalData.sh F
#./updateGapsInFundamentalData.sh F
#./fetchHistoricalData.sh F
#./updateGapsInHistoricalData.sh F
#./scanData.sh F
#./generateDataPatch.sh F F BER-HAM-HAN-MUN-GETTEX-XETR-DUS
#./applyDataPatch.sh F F.patch.matching_isin.json

#./calculate.sh STU DEU
#./generateScreenerReport.sh STU screen_filter_rank5F.json

#./calculate.sh F DEU
#./generateTickerReports.sh F plotSummary.json plotOverview.json
./generateScreenerReport.sh F screen_filter_rank5F.json
./generateComparisonReport.sh F compare_filterGIC_rank3F_template.json

#./generateDataPatch.sh STU F SWB
#./applyDataPatch.sh STU STU.patch.matching_isin.json

#./calculate.sh STU
#./generateTickerReports.sh STU plotSummary.json plotOverview.json
#./generateMarketReports.sh STU filterM7_rank3F.json
#./generateMarketReports.sh STU filterSTU_rank3F.json
