# EodHistoricalDataToolkit
A C++ toolkit written to analyze stock data provided by eodhistoricaldata.com

## Install

- CURL 7.81.0 
- TCLAP 1.4
- Boost-1.74.0
- nlohmann's json library (https://github.com/nlohmann/json)
- sciplot (https://sciplot.github.io)

The package manager APT provides CURL, TCLAP, and Boost for Ubuntu. The two other libraries need to be cloned to your machine. The CMakeFile includes for EodHistoricalDataToolkit includes entries for json/include and the sciplot directories that need to be manually entered.


## Set environment variables (Linux .bashrc entries)

Make sure that the EOD urls contain {YOUR_API_TOKEN} and {EXCHANGE_CODE} in the place of your api key and the actual exchange code. The fetch function will replace {YOUR_API_TOKEN} and {EXCHANGE_CODE} with values that are passed in as arguments to the fetch function.

> export EOD_API_TOKEN="YOUR_API_TOKEN"
> export EOD_DEMO="https://eodhistoricaldata.com/api/fundamentals/AAPL.US?api_token=demo"
> export EOD_EXCHANGES="https://eodhistoricaldata.com/api/exchanges-list/?api_token={YOUR_API_TOKEN}"
> export EOD_TICKERS="https://eodhistoricaldata.com/api/exchange-symbol-list/{EXCHANGE_CODE}?api_token={YOUR_API_TOKEN}"
> export EOD_EXCHANGE_BULK_DATA_TEST="http://eodhistoricaldata.com/api/bulk-fundamentals/{EXCHANGE_CODE}?api_token={YOUR_API_TOKEN}&fmt=json&offset=1&limit=10"

## Building the code

 1. Clone EodHistoricalDataToolkit to your local machine
 2. Make a build folder in EodHistoricalDataToolkit
 3. Open a terminal in the build folder and run 

    ccmake ..

 4. Manually enter entries for CUSTOM_NLOHMANN_INCLUDE_PATH and CUSTOM_SCIPLOT_INCLUDE_PATH. For example


 - BOOST_INC_DIR                    /usr/include
 - Boost_INCLUDE_DIR                /usr/include
 - CMAKE_BUILD_TYPE                 RelWithDebInfo
 - CMAKE_EXPORT_COMPILE_COMMANDS    ON
 - CMAKE_INSTALL_PREFIX             /usr/local
 - CUSTOM_NLOHMANN_INCLUDE_PATH     /home/mjhmilla/Work/code/investing/eodhistoricaldata/cpp/json/include
 - CUSTOM_SCIPLOT_INCLUDE_PATH      /home/mjhmilla/Work/code/investing/eodhistoricaldata/cpp/sciplot


## Processing steps:

1. Make sure these folders exist and are empty in the exchange folders. You'll have to manually add the exhange folder yourself, for example: EodHistoricalDataToolkit/data/STU for the Stuttgart stock exchange. Within an exchange folder you will need the following sub folders

 - fundamentalData
 - historicalData
 - calculateData
 - generateTickerReports
 - generateScreenerReport
 - generateComparisonReport

2. Fetch the list of exchanges

    ./fetchExchanges.sh

3. Fetch the list of exchange tickers from the Stuttgart stock exchange, where STU is the abbreviation used by EOD for the exchange.

    ./fetchExchangeTickers.sh STU

4. Fetch the fundamental data (typically cannot be done in one batch)

    ./fetchFundamentalData.sh STU

5. Fill the gaps in the fundamental data set

    ./updateGapsInFundamentalData.sh STU

6. Fetch the historical data

    ./fetchHistoricalData.sh STU

7. Fill the gaps in the historical data set

    ./updateGapsInHistoricalData.sh STU

8. Scan the fundamental data: looks for fundamental data files that are missing the PrimaryTicker field or the ISIN
    
    ./scanData.sh STU

9. Generate a patch for the files with missing data: this is done by using data from the TradingView.com. The three arguments are: the local exchange (STU - Stuttgart), a larger exchange that might have the missing data (F - Frankfurt), and the code for the local exchange on TradingView (SWB is the code for the Stuttgart stock exchange on TradingView).

    ./generateDataPatch.sh STU F SWB

10. Apply the patch by copying in the data found from the larger exchange/TradingView (STU.patch.matching_isin.json) into the fundamental data files

    ./applyDataPatch.sh STU STU.patch.matching_isin.json

11. Calculate all of the metrics used later. At the moment this primarily consists of metrics that are ultimately used to evaluate the value of the company (using the discounted cash flow model in Ch. 3 of The little book of Valuation by Aswath Damodaran). The second argument is the ISO3 format for the country from which you invest. This code is used to look up the inflation rate for your home country which is used to adjust the cost of capital when analyzing foreign companies.

    ./calculate.sh STU DEU

12. Generate the report for each ticker. This command will create a folder in, for example, EodHistoricalDataToolkit/data/STU/generateTickerReports for each stock using its primary ticker. Using GOOG_US as an example, you will find these files:

 - fig_GOOG_US_summary.pdf
 - fig_GOOG_US_overview.pdf
 - GOOG_US.tex
 - report_GOOG_US.tex

If you open a terminal within GOOG_US and call 

    pdflatex report_GOOG_US.tex

a pdf report will be generated that shows the summary figure, a description of the company, a number of example tables, and will conclude with a figure that shows some of the raw data which is helpful for further interpretation/debugging.

To generate these reports open a terminal from EodHistoricalDataToolkit and run the following bash script:

    ./generateTickerReports.sh STU plotSummary.json plotOverview.json

Note that plotSummary.json and plotOverview.json contain the information needed to configure the summary and overview plots. Note that these configuration files must be placed in the exchange folder (EodHistoricalDataToolkit/data/STU in this example).

13. Generate a report of a specific market. This report will filter out companies and then rank them according to the configuration file, which in this example is filterM7_rank3F.json:

    ./generateScreenerReport.sh STU filterM7_rank3F.json

This command will generate files in the generateScreenerReport folder that can be compiled into a pdf. For example, the above command will generate the following files in EodHistoricalDataToolkit/data/STU/generateScreenerReport:

 - summary_filterM7_rank3F_json.pdf
 - report_filterM7_rank3F_json.tex

Calling
 
    pdflatex report_filterM7_rank3F_json.tex

from a terminal within EodHistoricalDataToolkit/data/STU/generateScreenerReport will generate a market report report_filterM7_rank3F_json.pdf.

The file defines the market and some aspects of the market report. For example, filterM7_rank3F.json filters out the Magnificient 7 companies and then ranks them based on their price-to-value, residual cash-flow to enterprise value, the excess return-on-investment. If you examine the file you will also see an entry for market-capitalization but it is not included in the ranking because its weight is set to 0. Each company is ranked based on the sum of its individual metric rankings, each being scaled by the weight field specified in the configuration file.

The final market report will start with a summary plot for every ranking metric that shows an overview of every ticker in the market's historical dataset, plotted using box-and-whisker plots to avoid overwhelming the reader. These plots are followed by the ranked list of the companies along with their scores. Finally, the ticker reports for every company in the market are appended to this. Note that the report of a single market can be broken up into several reports: see the "report" section of filterSTU_rank3F.json for details.


## Licensing

All of the code and files in this repository are covered by the license mentioned in the SPDX file header which makes it possible to audit the licenses in this code base using the reuse lint command from https://api.reuse.software/. A full copy of the license can be found in the LICENSES folder. To keep the reuse tool happy even this file has a license:

SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
SPDX-License-Identifier: MIT


Files that cannot include SPDX commands in the header are have their licensing information mentioned in the REUSE.toml file. In addition, here is a brief summary:

- data/ISO-3166-Countries-with-Regional-Codes.json
    - Source: https://github.com/lukes/ISO-3166-Countries-with-Regional-Codes/        
    - License: CC-BY-SA-4.0 (c) 2011 Luke Duncalfe

- data/1980_2023_Corporate_Tax_Rates_Around_the_World_Tax_Foundation.csv
    - Source file: https://taxfoundation.org/wp-content/uploads/2023/12/1980_2023_Corporate_Tax_Rates_Around_the_World_Tax_Foundation.xlsx
    - Owner: Tax Foundation, 1325 G St NW, Suite 950, Washington, DC 20005 
    - License: CC-BY-NC 4.0, https://taxfoundation.org/copyright-notice/

- data/bondYieldTable.json
    - Source: https://fred.stlouisfed.org/series/DGS10
    - Owner: Federal Reserve Bank of St. Louis, One Federal Reserve Bank Plaza, St. Louis, MO 63102 
    - License: None listed

- The following files are generated using ctryprem.xlsx and data/ISO-3166-Countries-with-Regional-Codes.json
    - data/equityRiskPremiumByCountryDefault.csv
    - data/equityRiskPremiumByCountryDefault.json
    - data/equityRiskPremiumByCountry.json
- ctryprem.xlsx is not included in this repository but was used.
    - Source: https://pages.stern.nyu.edu/~adamodar/New_Home_Page/datafile/ctryprem.html
    - File: https://www.stern.nyu.edu/~adamodar/pc/datasets/ctryprem.xlsx
    - Owner: Aswath Damodaran, <ad4@stern.nyu.edu>
    - License: None listed
