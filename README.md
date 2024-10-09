# EodHistoricalDataToolkit
A c++ toolkit written to analyze stock data provided by eodhistoricaldata.com

# Install

- CURL
- TCLAP
- https://github.com/nlohmann/json
- https://sciplot.github.io/


# Set environment variables 

## Linux .bashrc entries

Make sure that the EOD urls contain {YOUR_API_TOKEN} and {EXCHANGE_CODE} in the place of your api key and the actual exchange code. The fetch function will replace {YOUR_API_TOKEN} and {EXCHANGE_CODE} with values that are passed in as arguments to the fetch function.

> export EOD_API_TOKEN="YOUR_API_TOKEN"
> export EOD_DEMO="https://eodhistoricaldata.com/api/fundamentals/AAPL.US?api_token=demo"
> export EOD_EXCHANGES="https://eodhistoricaldata.com/api/exchanges-list/?api_token={YOUR_API_TOKEN}"
> export EOD_TICKERS="https://eodhistoricaldata.com/api/exchange-symbol-list/{EXCHANGE_CODE}?api_token={YOUR_API_TOKEN}"
> export EOD_EXCHANGE_BULK_DATA_TEST="http://eodhistoricaldata.com/api/bulk-fundamentals/{EXCHANGE_CODE}?api_token={YOUR_API_TOKEN}&fmt=json&offset=1&limit=10"

# Processing steps:

1. Make sure these folders exist and are empty in the exchange (e.g. STU) folders:
 - fundamentalData
 - historicalData
 - calculateData
 - rankData
 - preprocess
 - postprocess

2. Fetch the list of exchanges
"""./fetchExchanges.sh"""

3. Fetch the list of exchange tickers from the Stuttgart stock exchange
"""./fetchExchangeTickers.sh STU"""

4. Fetch the fundamental data (typically cannot be done in one batch)
"""./fetchFundamentalData.sh STU"""

5. Fill the gaps in the fundamental data set
"""./updateGapsInFundamentalData.sh STU"""

6. Fetch the historical data
"""./fetchHistoricalData.sh STU"""

7. Fill the gaps in the historical data set
"""./updateGapsInHistoricalData.sh STU"""

8. Scan the fundamental data: looks for fundamental data files that are missing the PrimaryTicker field or the ISIN
"""./scanData.sh STU"""

9. Generate a patch for the files with missing data: this is done by using data from the TradingView.com. The three arguments are: the local exchange (STU - Stuttgart), a larger exchange that might have the missing data (F - Frankfurt), and the code for the local exchange on TradingView (SWB is the code for the Stuttgart stock exchange on TradingView).
"""./generateDataPatch.sh STU F SWB"""

10. Apply the patch by copying in the data found from the larger exchange/TradingView (STU.patch.matching_isin.json) into the fundamental data files
"""./applyDataPatch.sh STU STU.patch.matching_isin.json"""

11. Calculate all of the metrics used later. At the moment this primarily consists of metrics that are ultimately used to evaluate the value of the company (using the discounted cash flow model in Ch. 3 of The little book of Valuation by Aswath Damodaran). 
"""./calculate.sh STU"""

12. Generate the report for each ticker

"""./generateTickerReports.sh STU plotByValueRCashFlowExcessRoic.csv"""

12. [DEPRECATED] Rank the full list of companies according to the minimum of the sum of metric ranks. 

"""./rank.sh STU rankByValueRCashFlowExcessRoic.csv"""

Here we rank by the companies that do the best on priceToValue, residual free cash flow normalized by enterprise value, and the excess ROI since the file rankByValueRCashFlowExcessRoic.csv has these entries:

"""priceToValue,smallestIsBest"""
"""residualFreeCashFlowToEnterpriseValue,biggestIsBest"""
"""returnOnInvestedCapitalLessCostOfCapital,biggestIsBest"""

Note that rankByValueRCashFlowExcessRoic.csv needs to be in the exchange folder (STU in this case)

13. [DEPRECATED] Aggregate all of the necessary data to generate the illustrated reports. The arguments are the exchange (STU), the ranking criteria file (rankByValueRCashFlowExcessRoic.csv) and the output of the last step that contains the ranking which is stored in the rankData folder (rankByValueRCashFlowExcessRoic_ranking.json)

"""./preprocessing.sh STU rankByValueRCashFlowExcessRoic.csv rankByValueRCashFlowExcessRoic_ranking.json""" 

14. [DEPRECATED] Generate the figures and the LaTeX files needed to automatically create a pdf report summarizing this data

"""./postprocessing.sh STU plotByValueRCashFlowExcessRoic.csv rankByValueRCashFlowExcessRoic_report.json"""

This example report is generated in data/STU/postprocess/reportSTU.tex

There are a series of filtering options, here is one example where only the magnificient seven are included in the reportSTU.tex which can be compiled into a pdf using the tool LaTeX and the command pdflatex.

"""./postprocessingFilter.sh STU plotByValueRCashFlowExcessRoic.csv rankByValueRCashFlowExcessRoic_report.json filterByTicker_MagnificientSeven.json"""

where the file filterByTicker_MagnificientSeven.json is in the data/STU/ folder and contains

["GOOG.US","GOOGL.US","META.US","NVDA.US","AMZN.US","MSFT.US","AAPL.US","TSLA.US"]

This example command will generate a report in data/STU/postprocess/reportFilter.tex which can be compiled into a pdf using LaTeX.