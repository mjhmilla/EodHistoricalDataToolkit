#!/usr/bin/env bash
#./fetchExchanges.sh
#./fetchExchangeTickers.sh F

#./fetchFundamentalData.sh LSE
#./fetchHistoricalData.sh LSE
#./scanData.sh LSE
#./generateDataPatch.sh LSE LSE LSE
#./applyDataPatch.sh LSE LSE.patch.matching_isin.json


#./calculate.sh LSE GBR
./generateTickerReports.sh LSE generateTickerReports.json
./generateScreenerReport.sh LSE screen_filter_TownPriest.json
./generateScreenerReport.sh LSE screen_filter_TownPriest_10_Energy.json
./generateScreenerReport.sh LSE screen_filter_TownPriest_15_Materials.json
./generateScreenerReport.sh LSE screen_filter_TownPriest_20_Industrials.json
./generateScreenerReport.sh LSE screen_filter_TownPriest_25_ConsumerDiscretionary.json
./generateScreenerReport.sh LSE screen_filter_TownPriest_30_ConsumerStaples.json
./generateScreenerReport.sh LSE screen_filter_TownPriest_35_HeathCare.json
./generateScreenerReport.sh LSE screen_filter_TownPriest_40_Financials.json
./generateScreenerReport.sh LSE screen_filter_TownPriest_45_InformationTechnology.json
./generateScreenerReport.sh LSE screen_filter_TownPriest_50_CommunicationServices.json
./generateScreenerReport.sh LSE screen_filter_TownPriest_55_Utilities.json
./generateScreenerReport.sh LSE screen_filter_TownPriest_60_RealEstate.json



#./scanData.sh F
#./generateDataPatch.sh F F FWB
#./applyDataPatch.sh F F.patch.matching_isin.json

#./calculate.sh F DEU

#./generateTickerReports.sh F generateTickerReports.json
#./generateScreenerReport.sh F screen_filter_TownPriest.json
#./generateScreenerReport.sh F screen_filter_TownPriest_10_Energy.json
#./generateScreenerReport.sh F screen_filter_TownPriest_15_Materials.json
#./generateScreenerReport.sh F screen_filter_TownPriest_20_Industrials.json
#./generateScreenerReport.sh F screen_filter_TownPriest_25_ConsumerDiscretionary.json
#./generateScreenerReport.sh F screen_filter_TownPriest_30_ConsumerStaples.json
#./generateScreenerReport.sh F screen_filter_TownPriest_35_HeathCare.json
#./generateScreenerReport.sh F screen_filter_TownPriest_40_Financials.json
#./generateScreenerReport.sh F screen_filter_TownPriest_45_InformationTechnology.json
#./generateScreenerReport.sh F screen_filter_TownPriest_50_CommunicationServices.json
#./generateScreenerReport.sh F screen_filter_TownPriest_55_Utilities.json
#./generateScreenerReport.sh F screen_filter_TownPriest_60_RealEstate.json


#./generateScreenerReport.sh F screen_filter_PhilTownAswathDamodaran.json
#./generateScreenerReport.sh F screen_filter_PhilTownWithDividend.json
#./generateScreenerReport.sh F screen_filter_WilliamPriestShareholderYield.json

#./generateScreenerReport.sh LSE screen_filter_titans.json
#./generateScreenerReport.sh LSE screen_filter_rank5F.json


#./generateScreenerReport.sh F screen_filter_dividend.json
#./generateScreenerReport.sh F screen_filter_cyclicGrowth.json
#./generateScreenerReport.sh F screen_filter_expGrowth.json
#./generateScreenerReport.sh F screen_filterM7_rank3F.json
#./generateScreenerReport.sh F screen_dividend.json
#./generateScreenerReport.sh F screen_titins.json
#./generateScreenerReport.sh F screen_SvenCarlinCoveredStocks.json
#./generateScreenerReport.sh F screen_SuperInvestorConsensus.json
#./generateScreenerReport.sh F screen_FundSmithTopHoldings.json


