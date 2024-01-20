
#include <cstdio>
#include <fstream>
#include <string>
#include <cmath>

#include "date.h"
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include <filesystem>

#include "FinancialAnalysisToolkit.h"
#include "JsonFunctions.h"

int calcDifferenceInDaysBetweenTwoDates(std::string &dateA,
                                        const char* dateAFormat,
                                        std::string &dateB,
                                        const char* dateBFormat){

  std::istringstream dateStream(dateA);
  dateStream.exceptions(std::ios::failbit);
  date::sys_days daysA;
  dateStream >> date::parse(dateAFormat,daysA);

  dateStream.clear();
  dateStream.str(dateB);
  date::sys_days daysB;
  dateStream >> date::parse(dateBFormat,daysB);

  int daysDifference = (daysA-daysB).count();

  return daysDifference;

};

bool extractDatesOfClosestMatch(
              std::vector< std::string > &datesSetA,
              const char* dateAFormat,
              std::vector< std::string > &datesSetB,
              const char* dateBFormat,
              std::vector< unsigned int > &indicesOfClosestSetBDates,
              unsigned int maxNumberOfDaysInError){

    
  indicesOfClosestSetBDates.clear();

  bool validInput=true;

  //Get the indicies of the most recent and oldest indexes "%Y-%m-%d"
  int firstMinusLastSetA = 
    calcDifferenceInDaysBetweenTwoDates(
        datesSetA[0],
        dateAFormat,        
        datesSetA[datesSetA.size()-1],
        dateAFormat);


  //Get the indicies of the most recent and oldest indexes
  int firstMinusLastSetB = 
    calcDifferenceInDaysBetweenTwoDates(
        datesSetB[0],
        dateBFormat,        
        datesSetB[datesSetB.size()-1],
        dateBFormat);

  //The indices stored to match the order of datesSetA
  int indexSetBFirst = 0;
  int indexSetBLast = 0;
  int indexSetBDelta = 0;

  if(firstMinusLastSetB*firstMinusLastSetA > 0){
    indexSetBFirst = 0;
    indexSetBLast = static_cast<int>(datesSetB.size())-1;
    indexSetBDelta = 1;
  }else{
    indexSetBFirst = static_cast<int>(datesSetB.size())-1;
    indexSetBLast = 0;
    indexSetBDelta = -1;
  }


  int indexSetB=indexSetBFirst;
  int indexSetBEnd = indexSetBEnd+indexSetBDelta;

  std::string tempDateSetB("");
  std::string tempDateSetA("");

  for(unsigned int indexSetA=0; 
      indexSetA < datesSetA.size(); 
      ++indexSetA ){

    //Find the closest date in SetB data leading up to the fundamental
    //date     
    tempDateSetA = datesSetA[indexSetA];

    int daysInError;
    std::string lastValidDate;
    bool found=false;

    while( indexSetB != (indexSetBEnd) 
        && !found){

      tempDateSetB = datesSetB[indexSetB];

      int daysSetALessSetB = 
        calcDifferenceInDaysBetweenTwoDates(
            tempDateSetA,
            dateAFormat,        
            tempDateSetB,
            dateBFormat);

      //As long as the historical date is less than or equal to the 
      //fundamental date, increment
      if(daysSetALessSetB >= 0){
        found = true;
        daysInError = daysSetALessSetB;
        lastValidDate = tempDateSetB;
      }else{
        indexSetB += indexSetBDelta;
      }
    }

    //Go back to the last valid date and save its information
    if(indexSetB < datesSetB.size() 
    && indexSetB >= 0){  

      indicesOfClosestSetBDates.push_back(indexSetB);      
      if(daysInError >= maxNumberOfDaysInError){
        std::cout << "Date mismatch between historical data (" 
                  << tempDateSetB 
                  <<") and fundamental data (" << tempDateSetA 
                  <<") of " 
                  << daysInError 
                  <<" which is greater than the limit of " 
                  << maxNumberOfDaysInError << std::endl;                        
      }
    }else{
      std::cout << "  SetB data does not span date range of"
                    " SetA data."
                << std::endl;
      validInput=false;

    }
    
  }
  if(indicesOfClosestSetBDates.size() != datesSetA.size()){
    std::cout << "  Some dates in the fundamental data set could not be "
              << "found in the historical data set."
              << std::endl;
    validInput=false;
  }

  if(indicesOfClosestSetBDates.size() == 0){
    std::cout << "  SetB data does not contain any of the dates "
    "in the fundamental data set" << std::endl;
    validInput=false;
  }

  return validInput;
};



int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string fundamentalFolder;
  std::string historicalFolder;
  std::string eodFolder;
  std::string analyseFolder;
  bool analyzeQuarterlyData;
  std::string timePeriod;
  
  std::string defaultSpreadJsonFile;  
  std::string bondYieldJsonFile;  

  double defaultInterestCover;

  double defaultRiskFreeRate;
  double equityRiskPremium;
  double defaultBeta;

  std::string singleFileToEvaluate;

  double defaultTaxRate;
  int numberOfYearsToAverageCapitalExpenditures;
  int numberOfPeriodsToAverageCapitalExpenditures;
  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will analyze fundamental and end-of-data"
    "data (from https://eodhistoricaldata.com/) and "
    "write the results of the analysis to a json file in an output directory"
    ,' ', "0.0");


    TCLAP::ValueArg<std::string> fundamentalFolderInput("f",
      "fundamental_data_folder_path", 
      "The path to the folder that contains the fundamental data json files from "
      "https://eodhistoricaldata.com/ to analyze",
      true,"","string");

    cmd.add(fundamentalFolderInput);

    TCLAP::ValueArg<std::string> historicalFolderInput("p",
      "historical_data_folder_path", 
      "The path to the folder that contains the historical (price)"
      " data json files from https://eodhistoricaldata.com/ to analyze",
      true,"","string");

    cmd.add(historicalFolderInput);

    TCLAP::ValueArg<std::string> singleFileToEvaluateInput("i",
      "single_ticker_name", 
      "To evaluate a single ticker only, set the ticker name here.",
      true,"","string");

    cmd.add(singleFileToEvaluateInput);

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","exchange_code", 
      "The exchange code. For example: US",
      false,"","string");

    cmd.add(exchangeCodeInput);  

    TCLAP::ValueArg<std::string> defaultSpreadJsonFileInput("d",
      "default_spread_json_file_input", 
      "The path to the json file that contains a table relating interest"
      " coverage to default spread",false,"","string");

    cmd.add(defaultSpreadJsonFileInput);  

    TCLAP::ValueArg<std::string> bondYieldJsonFileInput("y",
      "bond_yield_file_input", 
      "The path to the json file that contains a long historical record"
      " of 10 year bond yield values",false,"","string");

    cmd.add(bondYieldJsonFileInput);  
    

    TCLAP::ValueArg<double> defaultInterestCoverInput("c",
      "default_interest_cover", 
      "The default interest cover that is used if an average value cannot"
      "be computed from the data provided. This can happen with older "
      "EOD records",
      false,2.5,"double");

    cmd.add(defaultInterestCoverInput);  

    TCLAP::ValueArg<double> defaultTaxRateInput("t",
      "default_tax_rate", 
      "The tax rate used if the tax rate cannot be calculated, or averaged, from"
      " the data",
      false,0.30,"double");

    cmd.add(defaultTaxRateInput);  


    TCLAP::ValueArg<double> defaultRiskFreeRateInput("r",
      "default_risk_free_rate", 
      "The risk free rate of return, which is often set to the return on "
      "a 10 year or 30 year bond as noted from Ch. 3 of Damodran.",
      false,0.025,"double");

    cmd.add(defaultRiskFreeRateInput);  

    TCLAP::ValueArg<double> equityRiskPremiumInput("e",
      "equity_risk_premium", 
      "The extra return that the stock should return given its risk. Often this"
      " is set to the historical incremental rate of return provided by the "
      " stock market relative to the bond market. In the U.S. this is somewhere"
      " around 4 percent between 1928 and 2010 as noted in Ch. 3 Damodran.",
      false,0.05,"double");

    cmd.add(equityRiskPremiumInput);  

    TCLAP::ValueArg<double> defaultBetaInput("b",
      "default_beta", 
      "The default beta value to use when one is not reported",
      false,1.0,"double");

    cmd.add(defaultBetaInput);  

    //numberOfYearsToAverageCapitalExpenditures
    TCLAP::ValueArg<int> numberOfYearsToAverageCapitalExpendituresInput("n",
      "number_of_years_to_average_capital_expenditures", 
      "Number of years used to evaluate capital expenditures."
      " Default value of 3 taken from Ch. 12 of Lev and Gu.",
      false,3,"int");

    cmd.add(numberOfYearsToAverageCapitalExpendituresInput);  

    TCLAP::SwitchArg quarterlyAnalysisInput("q","quarterly",
      "Analyze quarterly data", false);
    cmd.add(quarterlyAnalysisInput); 

    TCLAP::ValueArg<std::string> analyseFolderOutput("o","output_folder_path", 
      "The path to the folder that will contain the output json files "
      "produced by this analysis",
      true,"","string");

    cmd.add(analyseFolderOutput);

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    fundamentalFolder     = fundamentalFolderInput.getValue();
    singleFileToEvaluate = singleFileToEvaluateInput.getValue();
    historicalFolder      = historicalFolderInput.getValue();
    exchangeCode          = exchangeCodeInput.getValue();    
    analyseFolder         = analyseFolderOutput.getValue();
    analyzeQuarterlyData  = quarterlyAnalysisInput.getValue();

    defaultTaxRate      = defaultTaxRateInput.getValue();
    defaultRiskFreeRate = defaultRiskFreeRateInput.getValue();
    defaultBeta         = defaultBetaInput.getValue();
    equityRiskPremium   = equityRiskPremiumInput.getValue();


    defaultSpreadJsonFile = defaultSpreadJsonFileInput.getValue();
    bondYieldJsonFile     = bondYieldJsonFileInput.getValue();
    defaultInterestCover  = defaultInterestCoverInput.getValue();

    numberOfYearsToAverageCapitalExpenditures 
      = numberOfYearsToAverageCapitalExpendituresInput.getValue();              

    verbose             = verboseInput.getValue();

    if(analyzeQuarterlyData){
      timePeriod                  = Q;
      numberOfPeriodsToAverageCapitalExpenditures = 
        numberOfYearsToAverageCapitalExpenditures*4;

    }else{
      timePeriod                  = Y;
      numberOfPeriodsToAverageCapitalExpenditures = 
        numberOfYearsToAverageCapitalExpenditures;
    }   

    if(verbose){
      std::cout << "  Fundamental Data Folder" << std::endl;
      std::cout << "    " << fundamentalFolder << std::endl;

      std::cout << "  Historical Data Folder" << std::endl;
      std::cout << "    " << historicalFolder << std::endl;
      
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

      std::cout << "  Single file name to evaluate" << std::endl;
      std::cout << "    " << singleFileToEvaluate << std::endl;

      std::cout << "  Default Spread Json File" << std::endl;
      std::cout << "    " << defaultSpreadJsonFile << std::endl;

      std::cout << "  Default interest cover value" << std::endl;
      std::cout << "    " << defaultInterestCover << std::endl;

      std::cout << "  Analyze Quaterly Data" << std::endl;
      std::cout << "    " << analyzeQuarterlyData << std::endl;

      std::cout << "  Default tax rate" << std::endl;
      std::cout << "    " << defaultTaxRate << std::endl;

      std::cout << "  Annual default risk free rate" << std::endl;
      std::cout << "    " << defaultRiskFreeRate << std::endl;

      std::cout << "  Annual equity risk premium" << std::endl;
      std::cout << "    " << equityRiskPremium << std::endl;

      std::cout << "  Default beta value" << std::endl;
      std::cout << "    " << defaultBeta << std::endl;

      std::cout << "  Years to average capital expenditures " << std::endl;
      std::cout << "    " << numberOfYearsToAverageCapitalExpenditures 
                << std::endl;

      std::cout << "  Analyse Folder" << std::endl;
      std::cout << "    " << analyseFolder << std::endl;
    }
  } catch (TCLAP::ArgException &e)  // catch exceptions
	{ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }





  int numberOfYearsForTerminalValuation = 5;

  int maxDayErrorHistoricalData = 10; // Historical data has a resolution of 1 day
  int maxDayErrorBondYieldData  = 35; // Bond yield data has a resolution of 1 month
  bool zeroNanInShortTermDebt=true;
  bool zeroNansInResearchAndDevelopment=true;
  bool zeroNansInDividendsPaid = true;
  bool zeroNansInDepreciation = true;
  bool appendTermRecord=true;
  std::vector< std::string >  termNames;
  std::vector< double >       termValues;  

  bool loadSingleTicker=false;
  if(singleFileToEvaluate.size() > 0){
    loadSingleTicker = true;
  }

  std::string validFileExtension = exchangeCode;
  validFileExtension.append(".json");

  auto startingDirectory = std::filesystem::current_path();
  std::filesystem::current_path(fundamentalFolder);

  unsigned int count=0;

  //============================================================================
  // Load the default spread array 
  //============================================================================
  using json = nlohmann::ordered_json;    

  std::ifstream defaultSpreadFileStream(defaultSpreadJsonFile.c_str());
  json jsonDefaultSpread = nlohmann::ordered_json::parse(defaultSpreadFileStream);
  //std::cout << jsonDefaultSpread.at(1).at(2);
  if(verbose){
    std::cout << std::endl;
    std::cout << "default spread table" << std::endl;
    std::cout << '\t'
              << "Interest Coverage Interval" 
              << '\t'
              << '\t' 
              << "default spread" << std::endl;
    for(auto &row: jsonDefaultSpread["US"]["default_spread"].items()){
      for(auto &ele: row.value()){
        std::cout << '\t' << ele << '\t';
      }
      std::cout << std::endl;
    }
  }

  //============================================================================
  // Load the 10 year bond yield table 
  // Note: 1. Right now I only have historical data for the US, and so I'm
  //          approximating the bond yield of all countries using US data.
  //          This is probably approximately correct for wealthy developed
  //          countries that have access to international markets, but is 
  //          terrible for countries outside of this group.
  //       
  //============================================================================
  using json = nlohmann::ordered_json;    

  std::ifstream bondYieldFileStream(bondYieldJsonFile.c_str());
  json jsonBondYield = nlohmann::ordered_json::parse(bondYieldFileStream);

  if(verbose){
    std::size_t numberOfEntries = jsonBondYield["US"]["10y_bond_yield"].size();
    std::string startKey = jsonBondYield["US"]["10y_bond_yield"].begin().key();
    std::string endKey = (--jsonBondYield["US"]["10y_bond_yield"].end()).key();
    std::cout << std::endl;
    std::cout << "bond yield table with " 
              << numberOfEntries
              << " entries from "
              << startKey
              << " to "
              << endKey
              << std::endl;
    std::cout << "  Warning**" << std::endl; 
    std::cout << "    The 10 year bond yields from the US are being used to " << std::endl;          
    std::cout << "    approximate the bond yields from all countries. The "   << std::endl; 
    std::cout << "    bond yields, in turn, are being used to approximate"    << std::endl; 
    std::cout << "    the risk free rate which is used in the calculation "   << std::endl; 
    std::cout << "    of the cost of capital and the cost of debt."           << std::endl; 
    std::cout << std::endl;
  }



  //============================================================================
  //
  // Evaluate every file in the fundamental folder
  //
  //============================================================================  
  for ( const auto & entry 
          : std::filesystem::directory_iterator(fundamentalFolder)){


    bool validInput = true;

    //==========================================================================
    //Load the (primary) fundamental ticker file
    //==========================================================================
    std::string fileName=entry.path().filename();

    if(loadSingleTicker){
      fileName  = singleFileToEvaluate;
      size_t idx = singleFileToEvaluate.find(".json");
      if(idx == std::string::npos){
        fileName.append(".json");
      }
    }
    size_t lastIndex = fileName.find_last_of(".");
    std::string tickerName = fileName.substr(0,lastIndex);
    std::size_t foundExtension = fileName.find(validFileExtension);

    if( foundExtension != std::string::npos ){
        std::string updTickerName("");
        JsonFunctions::getPrimaryTickerName(fundamentalFolder, 
                                            fileName,
                                            updTickerName);
        if(verbose){
          std::cout << count << "." << '\t' << fileName << std::endl;
          if(updTickerName.compare(tickerName) != 0){
            std::cout << "  " << '\t' << updTickerName 
                      << " (PrimaryTicker) " << std::endl;
          }
        }

        if(updTickerName.length() > 0){        
            fileName = updTickerName;
            fileName.append(".json");
            tickerName = updTickerName;
        }else{
          validInput = false;
        }                                                        
    }

    //Try to load the fundamental file
    nlohmann::ordered_json fundamentalData;
    if(validInput){
      try{
        //Load the json file
        std::stringstream ss;
        ss << fundamentalFolder << fileName;
        std::string filePathName = ss.str();

        std::ifstream inputJsonFileStream(filePathName.c_str());
        fundamentalData = nlohmann::ordered_json::parse(inputJsonFileStream);
      }catch(const nlohmann::json::parse_error& e){
        std::cout << e.what() << std::endl;
        validInput=false;
      }
    }

    //Extract the list of entry dates for the fundamental data
    std::vector< std::string > datesFundamental;
    if(validInput){
      for(auto& el : fundamentalData[FIN][BAL][timePeriod].items()){
        datesFundamental.push_back(el.key());
      }
      if(datesFundamental.size()==0){
        std::cout << "  fundamental data contains no date entries" << std::endl;
        validInput=false;
      }
    }

    //==========================================================================
    //Load the (primary) historical (price) file
    //==========================================================================
    nlohmann::ordered_json historicalData;
    if(validInput){
      try{
        //Load the json file
        std::stringstream ss;
        ss << historicalFolder << fileName;
        std::string filePathName = ss.str();
        std::ifstream inputJsonFileStream(filePathName.c_str());
        historicalData = nlohmann::ordered_json::parse(inputJsonFileStream);
      }catch(const nlohmann::json::parse_error& e){
        std::cout << e.what() << std::endl;
        validInput=false;
      }
    }

    std::vector< std::string > datesHistorical;
    if(validInput){
      for(unsigned int i=0; i<historicalData.size(); ++i){
        std::string tempString("");
        JsonFunctions::getJsonString(historicalData[i]["date"],tempString);
        datesHistorical.push_back(tempString);
      }
      if(datesHistorical.size()==0){
        std::cout << "  historical data contains no date entries" << std::endl;
        validInput=false;
      }
    }    

    //Extract the list of closest entry dates for the historical data.
    //  We have to look for the closest date because sometimes stock
    //  exchanges are closed, and the date we need is missing.
    std::vector< unsigned int > indicesClosestHistoricalDates;    

    if(validInput){
      validInput = extractDatesOfClosestMatch(
                        datesFundamental,
                        "%Y-%m-%d",
                        datesHistorical,
                        "%Y-%m-%d",
                        indicesClosestHistoricalDates,
                        maxDayErrorHistoricalData);
    }

    //==========================================================================
    //Extract the closest order of the bond yields
    //==========================================================================
    std::vector< std::string > datesBondYields;
    for(auto& el : jsonBondYield["US"]["10y_bond_yield"].items()){
      datesBondYields.push_back(el.key());
    }
    std::vector< unsigned int > indicesClosestBondYieldDates;
    if(validInput){
      validInput = extractDatesOfClosestMatch(
                        datesFundamental,
                        "%Y-%m-%d",
                        datesBondYields,
                        "%Y-%m-%d",
                        indicesClosestBondYieldDates,
                        maxDayErrorBondYieldData);
    }    

    //==========================================================================
    //
    // Process these files, if all of the inputs are valid
    //
    //==========================================================================
    nlohmann::ordered_json analysis;
    if(validInput){

      std::vector< std::string > trailingPastPeriods;
      std::string previousTimePeriod("");
      unsigned int entryCount = 0;

      //========================================================================
      //Calculate 
      //  average tax rate
      //  average interest cover    
      //========================================================================
      std::vector< std::string > tmpNames;
      std::vector< double > tempValues;
      std::string tmpResultName("");
      unsigned int taxRateEntryCount = 0;
      double meanTaxRate = 0.;
      unsigned int meanInterestCoverEntryCount = 0;
      double meanInterestCover = 0.;

      //This assumes that the beta is the same for all time. This is 
      //obviously wrong, but I only have one data point for beta.      
      //
      //To do: compute Beta for every year and company using the data 
      //       that you have.      
      double beta = JsonFunctions::getJsonFloat(fundamentalData[GEN][TECH]["Beta"]);
      if(std::isnan(beta)){
        beta=defaultBeta;
      }


      //========================================================================
      // Evaluate the average tax rate and interest cover
      //========================================================================
      for( unsigned int indexFundamental=0;  
                        indexFundamental < datesFundamental.size(); 
                      ++indexFundamental){

        std::string date = datesFundamental[indexFundamental]; 

        double taxRateEntry = FinancialAnalysisToolkit::
                                calcTaxRate(fundamentalData, 
                                            date, 
                                            timePeriod.c_str(), 
                                            false, 
                                            tmpResultName,
                                            termNames,
                                            termValues);
        if(!std::isnan(taxRateEntry)){
          meanTaxRate += taxRateEntry;
          ++taxRateEntryCount;
        }
        double interestCover = FinancialAnalysisToolkit::
          calcInterestCover(fundamentalData,
                            date,
                            timePeriod.c_str(),
                            appendTermRecord,
                            termNames,
                            termValues);
        if(!std::isnan(interestCover) && !std::isinf(interestCover)){
          meanInterestCover += interestCover;
          ++meanInterestCoverEntryCount;
        }
      }        

      if(taxRateEntryCount > 0){
        meanTaxRate = meanTaxRate / static_cast<double>(taxRateEntryCount);
      }else{
        meanTaxRate = defaultTaxRate;
      }

      if(meanInterestCoverEntryCount > 0){
        meanInterestCover = meanInterestCover 
            / static_cast<double>(meanInterestCoverEntryCount);
      }else{
        meanInterestCover = defaultInterestCover;
      }


      //Get the index direction of older data
      int firstMinusLastFundamental = 
        calcDifferenceInDaysBetweenTwoDates(
            datesFundamental[0],
            "%Y-%m-%d",        
            datesFundamental[datesFundamental.size()-1],
            "%Y-%m-%d");
      int indexDeltaFundamentalPrevious = 0;
      if(firstMinusLastFundamental > 0){
        indexDeltaFundamentalPrevious = 1;
      }else{
        indexDeltaFundamentalPrevious = -1;
      }

      //=======================================================================
      //
      //  Calculate the metrics for every data entry in the file
      //
      //======================================================================= 
      for(int indexFundamental = 0; 
              indexFundamental < datesFundamental.size();
              ++indexFundamental){

        std::string date = datesFundamental[indexFundamental];        

        int indexPrevious = indexFundamental+indexDeltaFundamentalPrevious;
        if(indexPrevious >= 0 && indexPrevious < datesFundamental.size()){
          previousTimePeriod = datesFundamental[indexPrevious];
        }else{
          previousTimePeriod="";
        }

        termNames.clear();
        termValues.clear();

        //======================================================================
        //Update the list of past periods
        //======================================================================        
        trailingPastPeriods.clear();
        bool sufficentPastPeriods=true;
        int indexPastPeriods = indexFundamental;
        trailingPastPeriods.push_back(datesFundamental[indexPastPeriods]);

        for(unsigned int i=0; 
                         i<(numberOfPeriodsToAverageCapitalExpenditures-1); 
                       ++i)
        {
          indexPastPeriods += indexDeltaFundamentalPrevious;
          if(    indexPastPeriods >= 0 
              && indexPastPeriods < datesFundamental.size()){
            trailingPastPeriods.push_back(datesFundamental[indexPastPeriods]);
          }
        }

        //======================================================================
        //Evaluate the risk free rate as the yield on a 10 year US bond
        //======================================================================
        int indexBondYield = indicesClosestBondYieldDates[indexFundamental];
        std::string closestBondYieldDate= datesBondYields[indexBondYield]; 

        double bondYield = std::nan("1");
        try{
          bondYield = JsonFunctions::getJsonFloat(
              jsonBondYield["US"]["10y_bond_yield"][closestBondYieldDate]); 
          bondYield = bondYield * (0.01); //Convert from percent to decimal form      
        }catch( std::invalid_argument const& ex){
          std::cout << " Bond yield record (" << closestBondYieldDate << ")"
                    << " is missing a value. Reverting to the default"
                    << " risk free rate."
                    << std::endl;
          bondYield=defaultRiskFreeRate;
        }        

        double riskFreeRate = bondYield;

        //======================================================================
        //Evaluate the cost of equity
        //======================================================================        
        double annualCostOfEquityAsAPercentage = 
          riskFreeRate + equityRiskPremium*beta;

        double costOfEquityAsAPercentage=annualCostOfEquityAsAPercentage;
        if(analyzeQuarterlyData){
          costOfEquityAsAPercentage = costOfEquityAsAPercentage/4.0;
        }        

        termNames.push_back("costOfEquity_riskFreeRate");
        termNames.push_back("costOfEquity_equityRiskPremium");
        termNames.push_back("costOfEquity_beta");
        termNames.push_back("costOfEquity");

        termValues.push_back(riskFreeRate);
        termValues.push_back(equityRiskPremium);
        termValues.push_back(beta);
        termValues.push_back(costOfEquityAsAPercentage);

        //======================================================================
        //Evaluate the cost of debt
        //======================================================================        
        double interestCover = FinancialAnalysisToolkit::
          calcInterestCover(fundamentalData,
                            date,
                            timePeriod.c_str(),
                            appendTermRecord,
                            termNames,
                            termValues);
        if(std::isnan(interestCover) || std::isinf(interestCover)){
          interestCover = meanInterestCover;
        }

        double defaultSpread = std::nan("1");
        bool found=false;
        unsigned int i=0;        
        
        while(found == false 
              && i < jsonDefaultSpread["US"]["default_spread"].size()){
          
          double interestCoverLowerBound = JsonFunctions::getJsonFloat(
              jsonDefaultSpread["US"]["default_spread"].at(i).at(0));
          double interestCoverUpperBound = JsonFunctions::getJsonFloat(
              jsonDefaultSpread["US"]["default_spread"].at(i).at(1));
          double defaultSpreadIntervalValue = JsonFunctions::getJsonFloat(
              jsonDefaultSpread["US"]["default_spread"].at(i).at(2));


          if(interestCover >= interestCoverLowerBound
          && interestCover <= interestCoverUpperBound){
             defaultSpread = defaultSpreadIntervalValue;
            found=true;
          }
          ++i;
        } 
        if(std::isnan(defaultSpread) || std::isinf(defaultSpread)){
          std::cerr << "Error: the default spread has evaluated to nan or inf"
                       " which is only possible if the default spread json file"
                       " has errors in it. The default spread json file should"
                       " have 3 columns: interest cover lower bound, interest "
                       " cover upper bound, and default spread rate. Each row "
                       " specifies an interval. The intervals should be "
                       " continuous and cover from -10000 to 10000, likely with"
                       " very big intervals at the beginning and end. By default"
                       " data/defaultSpreadTable.json contains this information"
                       " which comes from "
                       " https://pages.stern.nyu.edu/~adamodar/New_Home_Page/datafile/ratings.html" 
                       " on January 2023. The 1st and 2nd columns come from the"
                       " table in the url, the 3rd column (rating) is ignored"
                       " while the final column is put in decimal format "
                       "(divide by 100). This table is appropriate for the U.S."
                       " and perhaps Europe, but is probably not correct for"
                       " the rest of the world."
                       << std::endl;
          std::abort();
        }       


        termNames.push_back("defaultSpread_interestCover");
        termNames.push_back("defaultSpread");
        termValues.push_back(interestCover);
        termValues.push_back(defaultSpread);

        std::string parentName = "afterTaxCostOfDebt_";
        double taxRate = FinancialAnalysisToolkit::
            calcTaxRate(fundamentalData,
                        date,
                        timePeriod.c_str(),
                        appendTermRecord,
                        parentName,
                        termNames,
                        termValues);
        if(std::isnan(taxRate) || std::isinf(taxRate)){
          taxRate = meanTaxRate;
          if(appendTermRecord){
            termValues[termValues.size()-1]=taxRate;
          }
        }
                        
        double afterTaxCostOfDebt = (riskFreeRate+defaultSpread)*(1.0-taxRate);
        termNames.push_back("afterTaxCostOfDebt_riskFreeRate");
        termNames.push_back("afterTaxCostOfDebt_defaultSpread");
        termNames.push_back("afterTaxCostOfDebt");
        
        termValues.push_back(riskFreeRate);
        termValues.push_back(defaultSpread);
        termValues.push_back(afterTaxCostOfDebt);

        //Evaluate the current total short and long term debt
        double shortLongTermDebtTotal = JsonFunctions::getJsonFloat(
          fundamentalData[FIN][BAL][timePeriod.c_str()][date.c_str()]
                  ["shortLongTermDebtTotal"]);
        
        //Evaluate the current market capitalization
        double commonStockSharesOutstanding = JsonFunctions::getJsonFloat(
          fundamentalData[FIN][BAL][timePeriod.c_str()][date.c_str()]
                  ["commonStockSharesOutstanding"]);

        unsigned int indexHistoricalData = 
          indicesClosestHistoricalDates[indexFundamental];   

        std::string closestHistoricalDate= datesHistorical[indexHistoricalData]; 

        double shareOpenValue = std::nan("1");
        try{
          shareOpenValue = JsonFunctions::getJsonFloat(
                              historicalData[ indexHistoricalData ]["open"]);       
        }catch( std::invalid_argument const& ex){
          std::cout << " Historical record (" << closestHistoricalDate << ")"
                    << " is missing an opening share price."
                    << std::endl;
        }

        double marketCapitalization = 
          shareOpenValue*commonStockSharesOutstanding;

        //Evaluate a weighted cost of capital
        double costOfCapital = 
          (costOfEquityAsAPercentage*marketCapitalization
          +afterTaxCostOfDebt*shortLongTermDebtTotal)
          /(marketCapitalization+shortLongTermDebtTotal);

        termNames.push_back("costOfCapital_shortLongTermDebtTotal");
        termNames.push_back("costOfCapital_commonStockSharesOutstanding");
        termNames.push_back("costOfCapital_shareOpenValue");
        termNames.push_back("costOfCapital_marketCapitalization");
        termNames.push_back("costOfCapital_costOfEquityAsAPercentage");
        termNames.push_back("costOfCapital_afterTaxCostOfDebt");
        termNames.push_back("costOfCapital");

        termValues.push_back(shortLongTermDebtTotal);
        termValues.push_back(commonStockSharesOutstanding);
        termValues.push_back(shareOpenValue);
        termValues.push_back(marketCapitalization);
        termValues.push_back(costOfEquityAsAPercentage);
        termValues.push_back(afterTaxCostOfDebt);
        termValues.push_back(costOfCapital);
        
        //======================================================================
        //Evaluate the metrics
        //======================================================================        

        double totalStockHolderEquity = JsonFunctions::getJsonFloat(
                fundamentalData[FIN][BAL][timePeriod.c_str()][date.c_str()]
                               ["totalStockholderEquity"]); 

        double roic = FinancialAnalysisToolkit::
          calcReturnOnInvestedCapital(fundamentalData,
                                      date,
                                      timePeriod.c_str(),
                                      zeroNansInDividendsPaid, 
                                      appendTermRecord, 
                                      termNames, 
                                      termValues);

        double roce = FinancialAnalysisToolkit::
          calcReturnOnCapitalDeployed(  fundamentalData,
                                        date,
                                        timePeriod.c_str(), 
                                        appendTermRecord, 
                                        termNames, 
                                        termValues);

        double grossMargin = FinancialAnalysisToolkit::
          calcGrossMargin(  fundamentalData,
                            date,
                            timePeriod.c_str(),
                            appendTermRecord,
                            termNames,
                            termValues);

        double operatingMargin = FinancialAnalysisToolkit::
          calcOperatingMargin(  fundamentalData,
                                date,
                                timePeriod.c_str(), 
                                appendTermRecord,
                                termNames,
                                termValues);          

        double cashConversion = FinancialAnalysisToolkit::
          calcCashConversionRatio(  fundamentalData,
                                    date,
                                    timePeriod.c_str(), 
                                    meanTaxRate,
                                    appendTermRecord,
                                    termNames,
                                    termValues);

        double debtToCapital = FinancialAnalysisToolkit::
          calcDebtToCapitalizationRatio(  fundamentalData,
                                          date,
                                          timePeriod.c_str(),
                                          zeroNanInShortTermDebt,
                                          appendTermRecord,
                                          termNames,
                                          termValues);

        double ownersEarnings = FinancialAnalysisToolkit::
          calcOwnersEarnings( fundamentalData, 
                              date, 
                              previousTimePeriod,
                              timePeriod.c_str(),
                              zeroNansInDepreciation,
                              appendTermRecord, 
                              termNames, 
                              termValues);  

        double residualCashFlow = std::nan("1");

        if(trailingPastPeriods.size() 
            == numberOfPeriodsToAverageCapitalExpenditures){

          residualCashFlow = FinancialAnalysisToolkit::
            calcResidualCashFlow( fundamentalData,
                                  date,
                                  timePeriod.c_str(),
                                  costOfEquityAsAPercentage,
                                  trailingPastPeriods,
                                  zeroNansInResearchAndDevelopment,
                                  appendTermRecord,
                                  termNames,
                                  termValues);
        }

        double freeCashFlowToEquity=std::nan("1");
        if(previousTimePeriod.length()>0){
          freeCashFlowToEquity = FinancialAnalysisToolkit::
            calcFreeCashFlowToEquity(fundamentalData, 
                                     date,
                                     previousTimePeriod,
                                     timePeriod.c_str(),
                                     zeroNansInDepreciation,
                                     appendTermRecord,
                                     termNames,
                                     termValues);
        }

        double freeCashFlowToFirm=std::nan("1");
        freeCashFlowToFirm = FinancialAnalysisToolkit::
          calcFreeCashFlowToFirm(fundamentalData, 
                                 date, 
                                 previousTimePeriod, 
                                 timePeriod.c_str(),
                                 meanTaxRate,
                                 zeroNansInDepreciation,
                                 appendTermRecord,
                                 termNames, 
                                 termValues);

        //Evaluation
        double presentValue = FinancialAnalysisToolkit::
            calcValuation(  fundamentalData,
                            date,
                            previousTimePeriod,
                            timePeriod.c_str(),
                            zeroNansInDividendsPaid,
                            zeroNansInDepreciation,
                            riskFreeRate,
                            costOfCapital,
                            numberOfYearsForTerminalValuation,
                            appendTermRecord,
                            termNames,
                            termValues);

        //it.c_str(), 
        nlohmann::ordered_json analysisEntry=nlohmann::ordered_json::object();        
        for( unsigned int i=0; i < termNames.size();++i){
          analysisEntry.push_back({termNames[i],
                                   termValues[i]});
        }

        analysis[date]= analysisEntry;        
        ++entryCount;
      }


      std::string outputFilePath(analyseFolder);
      std::string outputFileName(fileName.c_str());
      
      //Update the extension 
      std::string oldExt = ".json";
      std::string updExt = ".analysis.json";
      std::string::size_type pos = 0u;      
      pos = outputFileName.find(oldExt,pos);
      outputFileName.replace(pos,oldExt.length(),updExt);
      outputFilePath.append(outputFileName);

      std::ofstream outputFileStream(outputFilePath,
          std::ios_base::trunc | std::ios_base::out);
      outputFileStream << analysis;
      outputFileStream.close();
    }

    ++count;

    if(loadSingleTicker){
      std::cout << "Done evaluating: " << singleFileToEvaluate << std::endl;
      break;
    }
  }




  //For each file in the list
    //Open it
    //Get fundamentalData["General"]["PrimaryTicker"] and use this to generate an output file name
    //Create a new json file to contain the analysis
    //Call each analysis function individually to analyze the json file and
    //  to write the results of the analysis into the output json file
    //Write the analysis json file 

  return 0;
}
