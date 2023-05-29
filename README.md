# EodHistoricalDataToolkit
A c++ toolkit written to analyze stock data provided by eodhistoricaldata.com

# Install

- CURL
- TCLAP
- https://github.com/nlohmann/json
- https://sciplot.github.io/


# Set environment variables 

## Linux .bashrc entries

Make sure that the EOD urls contain {YOUR_API_KEY} and {EXCHANGE_CODE} in the place of your api key and the actual exchange code. The fetch function will replace {YOUR_API_KEY} and {EXCHANGE_CODE} with values that are passed in as arguments to the fetch function.

> export EOD_API_TOKEN="YOUR_API_TOKEN"
> export EOD_DEMO="https://eodhistoricaldata.com/api/fundamentals/AAPL.US?api_token=demo"
> export EOD_EXCHANGES="https://eodhistoricaldata.com/api/exchanges-list/?api_token={YOUR_API_KEY}"
> export EOD_TICKERS="https://eodhistoricaldata.com/api/exchange-symbol-list/{EXCHANGE_CODE}?api_token={YOUR_API_KEY}"
> export EOD_EXCHANGE_BULK_DATA_TEST="http://eodhistoricaldata.com/api/bulk-fundamentals/{EXCHANGE_CODE}?api_token={YOUR_API_TOKEN}&fmt=json&offset=1&limit=10"

# Example

./fetch -k $EOD_API_TOKEN -u $EOD_DEMO -f ../json/
