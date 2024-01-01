
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

int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string fundamentalFolder;
  std::string historicalFolder;
  std::string eodFolder;
  std::string analyseFolder;
  bool analyzeQuarterlyData;
  std::string timePeriod;
  
  std::string defaultSpreadJsonFile;  
  double defaultInterestCover;

  double riskFreeRate;
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

    TCLAP::ValueArg<std::string> singleFileToEvaluateInput("s",
      "single_ticker_name", 
      "To evaluate a single ticker only, set the full file name here.",
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


    TCLAP::ValueArg<double> riskFreeRateInput("r",
      "risk_free_rate", 
      "The risk free rate of return, which is often set to the return on "
      "a 10 year or 30 year bond as noted from Ch. 3 of Damodran.",
      false,0.025,"double");

    cmd.add(riskFreeRateInput);  

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
    riskFreeRate        = riskFreeRateInput.getValue();
    defaultBeta         = defaultBetaInput.getValue();
    equityRiskPremium   = equityRiskPremiumInput.getValue();


    defaultSpreadJsonFile = defaultSpreadJsonFileInput.getValue();
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

      std::cout << "  Annual risk free rate" << std::endl;
      std::cout << "    " << riskFreeRate << std::endl;

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


  int maxNumberOfDaysInError = 10;
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
    std::cout << "default spread table" << std::endl;
    for(auto &row: jsonDefaultSpread.items()){
      for(auto &ele: row.value()){
        std::cout << ele << '\t';
      }
      std::cout << std::endl;
    }
  }



  //============================================================================
  //
  // Evaluate every file in the fundamental folder
  //
  //============================================================================  
  for ( const auto & entry 
          : std::filesystem::directory_iterator(fundamentalFolder)){

    //Check to see if the input json file is valid and is for the primary
    //ticker

    bool validInput = true;

    std::string fileName=entry.path().filename();
    if(loadSingleTicker){
      fileName  = singleFileToEvaluate;
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
          std::cout << count << ".    " << fileName << std::endl;
          if(updTickerName.compare(tickerName) != 0){
            std::cout << "    " << updTickerName 
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

    //Try to load the historical file
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

    //Extract the list of entry dates for the fundamental data
    std::vector< std::string > datesFundamental;
    if(validInput){
      for(auto& el : fundamentalData[FIN][BAL][timePeriod].items()){
        datesFundamental.insert(datesFundamental.begin(),el.key());
      }
      if(datesFundamental.size()==0){
        std::cout << "  fundamental data contains no date entries" << std::endl;
        validInput=false;
      }
    }

    //Extract the list of closest entry dates for the historical data.
    //  We have to look for the closest date because sometimes stock
    //  exchanges are closed, and the date we need is missing.
    std::vector< unsigned int > indiciesDatesHistorical;    
    std::vector< std::string > datesHistorical;

    if(validInput){
      size_t indexHistorical=0;
      std::string tempDateHistorical("");
      std::string tempDateFundamental("");

      for(unsigned int indexFundamental=0; 
          indexFundamental < datesFundamental.size(); 
          ++indexFundamental ){

        //Find the closest date in Historical data leading up to the fundamental
        //date     
        tempDateFundamental = datesFundamental[indexFundamental];
        std::istringstream dateStream{tempDateFundamental};
        dateStream.exceptions(std::ios::failbit);
        date::sys_days daysFundamental;
        dateStream >> date::parse("%Y-%m-%d",daysFundamental);

        int daysInError;
        std::string lastValidDate;
        bool dateValid=true;

        while( indexHistorical < historicalData.size() && dateValid){

          JsonFunctions::getJsonString(
            historicalData[indexHistorical]["date"],tempDateHistorical);

          dateStream.clear();
          dateStream.str(tempDateHistorical);
          date::sys_days daysHistorical;
          dateStream >> date::parse("%Y-%m-%d",daysHistorical);

          //As long as the historical date is less than or equal to the 
          //fundamental date, increment
          if(daysHistorical <= daysFundamental){
            ++indexHistorical;
            daysInError = (daysFundamental-daysHistorical).count();
            lastValidDate = tempDateHistorical;
          }else{
            dateValid=false;
          }
        }

        //Go back to the last valid date and save its information
        if(indexHistorical < historicalData.size()){                      
          indiciesDatesHistorical.push_back(indexHistorical);
          datesHistorical.push_back(lastValidDate);          
          if(daysInError >= maxNumberOfDaysInError){
            std::cout << "Date mismatch between historical data (" 
                      << tempDateHistorical 
                      <<") and fundamental data (" << tempDateFundamental 
                      <<") of " 
                      << daysInError 
                      <<" which is greater than the limit of " 
                      << maxNumberOfDaysInError << std::endl;                        
          }
        }else{
          std::cout << "  Historical data does not span date range of"
                       " Fundamental data."
                    << std::endl;
          validInput=false;

        }
        
      }
      if(indiciesDatesHistorical.size() != datesFundamental.size()){
        std::cout << "  Some dates in the fundamental data set could not be "
                  << "found in the historical data set."
                  << std::endl;
        validInput=false;
      }

      if(indiciesDatesHistorical.size() == 0){
        std::cout << "  Historical data does not contain any of the dates "
        "in the fundamental data set" << std::endl;
        validInput=false;
      }
    }  


    //Process the file if its valid;
    nlohmann::ordered_json analysis;
    if(validInput){
      
      //========================================================================
      //Check 
      //  :that dates are ordered from oldest to newest.
      //  :Why? The code I've written to evaluate the 'previous' time period
      //        and the trailing last 3 time periods assumes this ordering.    
      //========================================================================

      //Check that the dates are ordered from oldest to newest
      std::istringstream timeStreamFirst(datesFundamental.front());
      std::istringstream timeStreamLast(datesFundamental.back());
      timeStreamFirst.imbue(std::locale("en_US.UTF-8"));
      timeStreamLast.imbue(std::locale("en_US.UTF-8"));
      std::tm firstTime = {};
      std::tm lastTime  = {};
      timeStreamFirst >> std::get_time(&firstTime,"%Y-%m-%d");
      timeStreamLast  >> std::get_time(&lastTime,"%Y-%m-%d");
      if(timeStreamFirst.fail() || timeStreamLast.fail()){
        std::cerr << "Error: converting date strings to double values failed"
                  << std::endl;
        validInput = false;
      }
      std::mktime(&firstTime);
      std::mktime(&lastTime);
      
      //Make sure the first time is smaller than the last time
      if(firstTime.tm_year > lastTime.tm_year){
        std::cerr << "Error: time entries are in the opposite order than expected"
                  << std::endl;
        validInput = false;
      }
    }

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
      double beta = JsonFunctions::getJsonFloat(fundamentalData[GEN][TECH]["Beta"]);
      if(std::isnan(beta)){
        beta=defaultBeta;
      }
      double annualCostOfEquityAsAPercentage = 
          riskFreeRate + equityRiskPremium*beta;

      double costOfEquityAsAPercentage=annualCostOfEquityAsAPercentage;
      if(analyzeQuarterlyData){
        costOfEquityAsAPercentage = costOfEquityAsAPercentage/4.0;
      }

      //========================================================================
      // Evaluate the average tax rate and interest cover
      //========================================================================
      for( auto& it : datesFundamental){        
        std::string date = it;           
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

      //=======================================================================
      //
      //  Calculate the metrics for every data entry in the file
      //
      //=======================================================================      
      for( auto& it : datesFundamental){
        std::string date = it;        


        termNames.clear();
        termValues.clear();

        //======================================================================
        //Evaluate the cost of equity
        //======================================================================        
        termNames.push_back("costOfEquityAsAPercentage_riskFreeRate");
        termNames.push_back("costOfEquityAsAPercentage_equityRiskPremium");
        termNames.push_back("costOfEquityAsAPercentage_beta");
        termNames.push_back("costOfEquityAsAPercentage");

        termValues.push_back(riskFreeRate);
        termValues.push_back(equityRiskPremium);
        termValues.push_back(beta);
        termValues.push_back(costOfEquityAsAPercentage);

        //======================================================================
        //Evaluate the cost of debt
        //======================================================================        
        double interestCover = FinancialAnalysisToolkit::
          calcInterestCover(fundamentalData,
                            it,
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
        while(found == false && i < jsonDefaultSpread.size()){
          if(interestCover >= jsonDefaultSpread.at(i).at(0) 
              && interestCover <= jsonDefaultSpread.at(i).at(1)){
            defaultSpread = jsonDefaultSpread.at(i).at(2);
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
                        it,
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

        unsigned int indexHistoricalData = indiciesDatesHistorical[entryCount];      
        std::string closestHistoricalDate= datesHistorical[entryCount]; 

        double shareOpenValue = std::nan("1");
        try{
          shareOpenValue = JsonFunctions::getJsonFloat(
                              historicalData[ indexHistoricalData ]["open"]);       
        }catch( std::invalid_argument const& ex){
          std::cout << " Historical record (" << closestHistoricalDate << ")"
                    << " is missing an opening share price."
                    << std::endl;
        }

        double marketCapitalization = shareOpenValue*commonStockSharesOutstanding;

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
        //Update the list of past periods
        //======================================================================        

        trailingPastPeriods.push_back(date);

        if(trailingPastPeriods.size() 
          > numberOfPeriodsToAverageCapitalExpenditures){
          trailingPastPeriods.erase(trailingPastPeriods.begin());

        }

        if(trailingPastPeriods.size() > 
                numberOfPeriodsToAverageCapitalExpenditures){
          std::cerr << "Error: trailingPastPeriods has exceeded"
                    << " numberOfPeriodsToAverageCapitalExpenditures" 
                    << std::endl;
          abort();
        }
        
        //======================================================================
        //Evaluate the metrics
        //======================================================================        

        double totalStockHolderEquity = JsonFunctions::getJsonFloat(
          fundamentalData[FIN][BAL][timePeriod.c_str()][it.c_str()]
                  ["totalStockholderEquity"]); 


        double roic = FinancialAnalysisToolkit::
          calcReturnOnInvestedCapital(fundamentalData,
                                      it,
                                      timePeriod.c_str(),
                                      zeroNansInDividendsPaid, 
                                      appendTermRecord, 
                                      termNames, 
                                      termValues);

        double roce = FinancialAnalysisToolkit::
          calcReturnOnCapitalDeployed(  fundamentalData,
                                        it,
                                        timePeriod.c_str(), 
                                        appendTermRecord, 
                                        termNames, 
                                        termValues);

        double grossMargin = FinancialAnalysisToolkit::
          calcGrossMargin(  fundamentalData,
                            it,
                            timePeriod.c_str(),
                            appendTermRecord,
                            termNames,
                            termValues);

        double operatingMargin = FinancialAnalysisToolkit::
          calcOperatingMargin(  fundamentalData,
                                it,
                                timePeriod.c_str(), 
                                appendTermRecord,
                                termNames,
                                termValues);          

        double cashConversion = FinancialAnalysisToolkit::
          calcCashConversionRatio(  fundamentalData,
                                    it,
                                    timePeriod.c_str(), 
                                    meanTaxRate,
                                    appendTermRecord,
                                    termNames,
                                    termValues);

        double debtToCapital = FinancialAnalysisToolkit::
          calcDebtToCapitalizationRatio(  fundamentalData,
                                          it,
                                          timePeriod.c_str(),
                                          zeroNanInShortTermDebt,
                                          appendTermRecord,
                                          termNames,
                                          termValues);

        double ownersEarnings = FinancialAnalysisToolkit::
          calcOwnersEarnings( fundamentalData, 
                              it, 
                              previousTimePeriod,
                              timePeriod.c_str(),
                              appendTermRecord, 
                              termNames, 
                              termValues);  

        double residualCashFlow = std::nan("1");

        if(trailingPastPeriods.size() 
            == numberOfPeriodsToAverageCapitalExpenditures){

          residualCashFlow = FinancialAnalysisToolkit::
            calcResidualCashFlow( fundamentalData,
                                  it,
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
                                     it,
                                     previousTimePeriod,
                                     timePeriod.c_str(),
                                     appendTermRecord,
                                     termNames,
                                     termValues);
        }

        double freeCashFlowToFirm=std::nan("1");
        freeCashFlowToFirm = FinancialAnalysisToolkit::
          calcFreeCashFlowToFirm(fundamentalData, 
                                 it, 
                                 previousTimePeriod, 
                                 timePeriod.c_str(),
                                 meanTaxRate,
                                 appendTermRecord,
                                 termNames, 
                                 termValues);

        //it.c_str(), 
        nlohmann::ordered_json analysisEntry=nlohmann::ordered_json::object();        
        for( unsigned int i=0; i < termNames.size();++i){
          analysisEntry.push_back({termNames[i],
                                   termValues[i]});
        }

        /*
        json analysisEntry = json::object( 
                          {                             
                            {"residualCashFlow",residualCashFlow},
                            {"ownersEarnings",ownersEarnings},
                            {"returnOnInvestedCapital",roic},
                            {"returnOnCapitalDeployed", roce},
                            {"grossMargin",grossMargin},
                            {"operatingMargin",operatingMargin},
                            {"cashConversionRatio",cashConversion},
                            {"debtToCapitalRatio",debtToCapital},
                            {"interestCover",interestCover},
                            {"totalStockHolderEquity",totalStockHolderEquity},
                            {"freeCashFlowToFirm",freeCashFlowToFirm},
                          }
                        );
        */
        analysis[it]= analysisEntry;
        previousTimePeriod = date;
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
