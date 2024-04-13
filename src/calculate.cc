
#include <cstdio>
#include <fstream>
#include <string>
#include <cmath>
#include <limits>

#include "date.h"
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include <filesystem>

#include "FinancialAnalysisToolkit.h"
#include "JsonFunctions.h"


struct TaxFoundationDataSet{
  std::vector< int > index;
  std::vector< int > year;
  std::vector< std::string > CountryISO2;
  std::vector< std::string > CountryISO3;
  std::vector< std::string > continent;
  std::vector< std::string > country;
  std::vector< std::vector < double > > taxTable; 
};

double getTaxRateFromTable(std::string& countryISO2, int year, int yearMin, 
                           TaxFoundationDataSet &taxDataSet){

  double taxRate = std::numeric_limits<double>::quiet_NaN();                              

  //Get the country index
  bool foundCountry=false;
  int indexCountry = 0;
  while(indexCountry < taxDataSet.CountryISO3.size() && !foundCountry ){
    if(countryISO2.compare(taxDataSet.CountryISO2[indexCountry])==0){
      foundCountry=true;
    }else{
      ++indexCountry;
    }
  }  

  int indexYearMin  = -1;
  int indexYear     = -1;

  if(foundCountry){
    //Get the closest index to yearMin
    int index         = 0;
    int err           = 1;

    int errYear       = std::numeric_limits<int>::max();

    while(index < taxDataSet.year.size() && err > 0 ){
      err = yearMin - taxDataSet.year[index];
      if(err < errYear ){
        errYear = err;
        indexYearMin = index;
      }else{
        ++index;
      }
    }  
    //Get the closest index to year
    index     = indexYearMin;
    err       = 1;
    errYear   = std::numeric_limits<int>::max();

    while(index < taxDataSet.year.size() && err > 0 ){
      err = year - taxDataSet.year[index];
      if(err < errYear ){
        errYear = err;
        indexYear = index;
      }else{
        ++index;
      }
    }  
  }

  //Scan backwards from the most recent acceptable year to year min
  //and return the first valid tax rate
  if(foundCountry && (indexYear > 0 && indexYearMin > 0)){
    bool validIndex=true;

    int index = indexYear;
    taxRate = taxDataSet.taxTable[indexCountry][index];
    while( std::isnan(taxRate) && index > indexYearMin && validIndex){
      --index;
      if(index < 0){
        validIndex = false;
      }else{
        taxRate = taxDataSet.taxTable[indexCountry][index];   
      }
    }
    
  }

  return taxRate;

}

bool loadTaxFoundationDataSet(std::string &fileName, 
                              TaxFoundationDataSet &dataSet){

  bool validFormat=false;                                  
  std::ifstream file(fileName);

  if(file.is_open()){
    validFormat=true;
    std::string line;
    std::string entry;

    //First line is the header: check entries + read in years    
    std::getline(file,line); 

    std::size_t idx0=0;
    std::size_t idx1=0;
    int column = 0;
    std::vector< double > dataRow;
    dataRow.clear();

    idx1 = line.find_first_of(',',idx0);
    entry = line.substr(idx0,idx1-idx0);

    do{
        if(column <= 4){
          switch(column){
            case 1:{
                if(entry.compare("iso_2") != 0){
                  validFormat=false;
                }
            }break;
            case 2:{
              if(entry.compare("iso_3") != 0){
                  validFormat=false;
              }
            } break;
            case 3:{
              if(entry.compare("continent") != 0){
                  validFormat=false;
              }
            }break;
            case 4:{
              if(entry.compare("country") != 0){
                  validFormat=false;
              }                
            }break;
          };
        }else{
          if(entry.compare("NA") == 0){
            dataSet.year.push_back(-1);
            validFormat=false;
          }else{
            dataSet.year.push_back(std::stoi(entry));
          }
        }

        idx0 = idx1+1;
        idx1 = line.find_first_of(',',idx0);
        entry = line.substr(idx0,idx1-idx0);

        ++column;
    }while(    idx0 !=  std::string::npos 
            && idx1 !=  std::string::npos 
            && validFormat);


    //Read in the data table
    while(std::getline(file,line) && validFormat){

        idx0=0;
        idx1=0;
        dataRow.clear();
        column = 0;
        idx1 = line.find_first_of(',',idx0);
        entry = line.substr(idx0,idx1-idx0);

        do{


          if(column <= 4){
            switch(column){
              case 0:{
                  dataSet.index.push_back(std::stoi(entry));
              } break;
              case 1:{
                  dataSet.CountryISO2.push_back(entry);
              }break;
              case 2:{
                dataSet.CountryISO3.push_back(entry);
              } break;
              case 3:{
                dataSet.continent.push_back(entry);
              }break;
              case 4:{
                dataSet.country.push_back(entry);                
              }break;
            };
          }else{
            if(entry.compare("NA") == 0){
              dataRow.push_back(std::nan("1"));
            }else{
              dataRow.push_back(std::stod(entry));
            }
          }
          idx0 = idx1+1;
          idx1 = line.find_first_of(',',idx0);
          entry = line.substr(idx0,idx1-idx0);

          if(entry.find_first_of('"') != std::string::npos){
            //Opening bracket
            idx1 = line.find_first_of('"',idx0);
            //Closing bracket
            idx1 = line.find_first_of('"',idx1+1);
            //Final comma
            idx1 = line.find_first_of(',',idx1);
            entry = line.substr(idx0,idx1-idx0);
          }

          ++column;
        }while( idx0 !=  std::string::npos && idx1 !=  std::string::npos );

        dataSet.taxTable.push_back(dataRow);


    }
    file.close();
  }else{
   std::cout << " Warning: Reverting to default tax rate. Failed to " 
             << " read in " << fileName << std::endl; 
  }

  return validFormat;
};


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
              unsigned int maxNumberOfDaysInError,
              std::vector< std::string > &datesCommonToAandB,
              std::vector< unsigned int > &indicesOfCommonSetADates,
              std::vector< unsigned int > &indicesOfCommonSetBDates ){

    
  indicesOfCommonSetADates.clear();
  indicesOfCommonSetBDates.clear();
  datesCommonToAandB.clear();

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


  int indexSetB    = indexSetBFirst;
  int indexSetBEnd = indexSetBLast+indexSetBDelta;

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

    while( indexSetB != (indexSetBEnd) && !found){


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
    if(indexSetB < datesSetB.size() && indexSetB >= 0 
        && daysInError <= maxNumberOfDaysInError){  
        indicesOfCommonSetADates.push_back(indexSetA);
        indicesOfCommonSetBDates.push_back(indexSetB);
        datesCommonToAandB.push_back(tempDateSetA);           
    }
    
  }

  if(datesCommonToAandB.size() == 0){
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
  std::string corpTaxesWorldFile;

  double defaultInterestCover;

  double defaultRiskFreeRate;
  double equityRiskPremium;
  double defaultBeta;

  std::string singleFileToEvaluate;

  double defaultTaxRate;
  int numberOfYearsToAverageCapitalExpenditures;
  int numberOfPeriodsToAverageCapitalExpenditures;

  int numberOfYearsForTerminalValuation;
  int maxDayErrorTabularData;
  bool relaxedCalculation;

  double matureFirmFractionOfDebtCapital=0;

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
      false,"","string");
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
      "The tax rate used if the tax rate cannot be found in tabular data."
      " The default is 0.256 (25.6%) which is the world wide average"
      "weighted by GDP reported by the tax foundation.",
      false,0.256,"double");
    cmd.add(defaultTaxRateInput);  

    TCLAP::ValueArg<std::string> corpTaxesWorldFileInput("w","global_corporate_tax_rate_file", 
      "Corporate taxes reported around the world from the tax foundation"
      " in csv format (https://taxfoundation.org/data/all/global/corporate-tax-rates-by-country-2023/)",
      false,"","string");
    cmd.add(corpTaxesWorldFileInput);  
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

    //Default value from Ch. 3 Damodaran
    TCLAP::ValueArg<double> matureFirmFractionOfDebtCapitalInput("u",
      "mature_firm_fraction_debt_capital", 
      "The fraction of capital from debt for a mature firm.",
      false,0.2,"double");
    cmd.add(matureFirmFractionOfDebtCapitalInput);  

    TCLAP::ValueArg<int> numberOfYearsToAverageCapitalExpendituresInput("n",
      "number_of_years_to_average_capital_expenditures", 
      "Number of years used to evaluate capital expenditures."
      " Default value of 3 taken from Ch. 12 of Lev and Gu.",
      false,3,"int");
    cmd.add(numberOfYearsToAverageCapitalExpendituresInput);  

    TCLAP::ValueArg<int>numberOfYearsForTerminalValuationInput("m",
      "number_of_years_for_terminal_valuation_calculation", 
      "Number of years for terminal valuation calculation."
      " Default value of 5 taken from Ch. 3 of Damodran.",
      false,5,"int");
    cmd.add(numberOfYearsForTerminalValuationInput); 


    TCLAP::ValueArg<int>maxDayErrorTabularDataInput("a",
      "day_error", 
      "The bond value and stock value tables do not have entries for"
      " every day. This parameter specifies the maximum amount of error"
      " that is acceptable. Note: the bond table is sometimes missing a "
      " 30 days of data while the stock value tables are sometimes missing"
      " 10 days of data.",
      false,35,"int");
    cmd.add(maxDayErrorTabularDataInput); 

    TCLAP::SwitchArg relaxedCalculationInput("l","relaxed",
      "Relaxed calculation: nulls for some values (short term debt,"
      " research and development, dividends, depreciation) are set to "
      " zero, while substitute calculations are used to replace "
      " null values (shortLongDebt replaced with longDebt)", false);
    cmd.add(relaxedCalculationInput); 

    TCLAP::SwitchArg quarterlyAnalysisInput("q","quarterly",
      "Analyze quarterly data. Caution: this is not yet been tested.", false);
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
    corpTaxesWorldFile  = corpTaxesWorldFileInput.getValue();
    defaultRiskFreeRate = defaultRiskFreeRateInput.getValue();
    defaultBeta         = defaultBetaInput.getValue();
    equityRiskPremium   = equityRiskPremiumInput.getValue();


    defaultSpreadJsonFile = defaultSpreadJsonFileInput.getValue();
    bondYieldJsonFile     = bondYieldJsonFileInput.getValue();
    defaultInterestCover  = defaultInterestCoverInput.getValue();

    matureFirmFractionOfDebtCapital = 
      matureFirmFractionOfDebtCapitalInput.getValue();

    numberOfYearsToAverageCapitalExpenditures 
      = numberOfYearsToAverageCapitalExpendituresInput.getValue();              

    numberOfYearsForTerminalValuation
      = numberOfYearsForTerminalValuationInput.getValue();

    maxDayErrorTabularData
      = maxDayErrorTabularDataInput.getValue();

    relaxedCalculation  = relaxedCalculationInput.getValue() ;

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

      std::cout << "  Bond Yield Json File" << std::endl;
      std::cout << "    " << bondYieldJsonFile << std::endl;

      std::cout << "  Default interest cover value" << std::endl;
      std::cout << "    " << defaultInterestCover << std::endl;

      std::cout << "  Analyze Quaterly Data" << std::endl;
      std::cout << "    " << analyzeQuarterlyData << std::endl;

      std::cout << "  Default tax rate" << std::endl;
      std::cout << "    " << defaultTaxRate << std::endl;

      std::cout << "  Corporate tax rate file from https://taxfoundation.org"
                << std:: endl;
      std::cout << "    " << corpTaxesWorldFile << std::endl;

      std::cout << "  Annual default risk free rate" << std::endl;
      std::cout << "    " << defaultRiskFreeRate << std::endl;

      std::cout << "  Annual equity risk premium" << std::endl;
      std::cout << "    " << equityRiskPremium << std::endl;

      std::cout << "  Default beta value" << std::endl;
      std::cout << "    " << defaultBeta << std::endl;

      std::cout << "  Assumed mature firm fraction of capital from debt" << std::endl;
      std::cout << "    " << matureFirmFractionOfDebtCapital << std::endl;

      std::cout << "  Number of years to use when evaluating the terminal value " 
                << std::endl;
      std::cout << "    " << numberOfYearsForTerminalValuation 
                << std::endl;

      std::cout << "  Maximum number of days in error allowed for tabular " 
                << "data of bond yields and historical stock prices " 
                << std::endl;                
      std::cout << "    " << maxDayErrorTabularData 
                << std::endl;    

      std::cout << "  Using relaxed calculations?" << std::endl;
      std::cout << relaxedCalculation << std::endl;

      std::cout << "  Analyse Folder" << std::endl;
      std::cout << "    " << analyseFolder << std::endl;
    }
  } catch (TCLAP::ArgException &e)  // catch exceptions
	{ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }

  int acceptableBackwardsYearErrorForTaxRate = 5;
  //This defines how far back in time data from the tax table can be taken
  //to approximate the tax of this year.

  int maxDayErrorHistoricalData = maxDayErrorTabularData; 
  // Historical data has a resolution of 1 day
  
  int maxDayErrorBondYieldData  = maxDayErrorTabularData; 
  // Bond yield data has a resolution of 1 month

  //bool relaxedCalculation = true;
  //Some of entries in EODs data base are frequently null because the values
  //are not reported in the original financial statements. There are two
  //ways to handle this case:
  //
  // relaxedCalculation = true: 
  //  Set (some) of these null values to zero, and substitute some combined 
  //  quantities that are null (short-long-term debt) with an approximate them 
  //  with one term (long term debt).
  //
  // relaxedCalculation = false: 
  //  Use the data as is directly from EOD without any modification.

  bool zeroNanInShortTermDebt                 = relaxedCalculation;
  bool replaceNanInShortLongDebtWithLongDebt  = relaxedCalculation;
  bool zeroNansInResearchAndDevelopment       = relaxedCalculation;
  bool zeroNansInDividendsPaid                = relaxedCalculation;
  bool zeroNansInDepreciation                 = relaxedCalculation;
  bool appendTermRecord                       = relaxedCalculation;
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
  // Load the corporate tax rate table
  //============================================================================
  bool flag_usingTaxTable=false;
  TaxFoundationDataSet corpWorldTaxTable;
  if(corpTaxesWorldFile.length() > 0){
    flag_usingTaxTable=true;
    bool validFormat = 
      loadTaxFoundationDataSet(corpTaxesWorldFile,corpWorldTaxTable);

    if(!validFormat){
      flag_usingTaxTable=false;
      std::cout << "Warning: could not load the world corporate tax rate file "
                << corpTaxesWorldFile << std::endl;
      std::cout << "Reverting to the default rate " << std::endl;
    }
  }
  //============================================================================
  // Load the default spread array 
  //============================================================================
  using json = nlohmann::ordered_json;    

  std::ifstream defaultSpreadFileStream(defaultSpreadJsonFile.c_str());
  json jsonDefaultSpread = 
    nlohmann::ordered_json::parse(defaultSpreadFileStream);
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
  int validFileCount=0;
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
        ++validFileCount;                                            
        if(verbose){
          std::cout << validFileCount << "." << '\t' << fileName << std::endl;
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
          if(verbose){
            std::cout << "  Skipping: PrimaryTicker is not listed" << std::endl; 
          }
        }                                                        
    }

    //Try to load the fundamental file
    nlohmann::ordered_json fundamentalData;
    if(validInput){
      validInput = JsonFunctions::loadJsonFile(fileName, fundamentalFolder, 
                                               fundamentalData, verbose);
    }

    //Extract the list of entry dates for the fundamental data
    std::vector< std::string > datesFundamental;
    if(validInput){
      for(auto& el : fundamentalData[FIN][BAL][timePeriod].items()){
        datesFundamental.push_back(el.key());
      }
      if(datesFundamental.size()==0){
        validInput=false;
        if(verbose){
          std::cout << "  Skipping: fundamental data contains no date entries" 
                    << std::endl; 
        }
      }
    }
    //Extract the countryName and ISO
    std::string countryName;
    std::string countryISO2;
    if(validInput){
      JsonFunctions::getJsonString(fundamentalData[GEN]["CountryName"],
                                   countryName);
      JsonFunctions::getJsonString(fundamentalData[GEN]["CountryISO"],
                                   countryISO2);

    }

    //==========================================================================
    //Load the (primary) historical (price) file
    //==========================================================================
    nlohmann::ordered_json historicalData;
    if(validInput){
      validInput=JsonFunctions::loadJsonFile(fileName, historicalFolder, 
                                            historicalData, verbose);
    }

    std::vector< std::string > datesHistorical;
    if(validInput){
      for(unsigned int i=0; i<historicalData.size(); ++i){
        std::string tempString("");
        JsonFunctions::getJsonString(historicalData[i]["date"],tempString);
        datesHistorical.push_back(tempString);
      }
      if(datesHistorical.size()==0){
        validInput=false;
        if(verbose){
          std::cout << "  Skipping: historical data contains no date entries" 
                    << std::endl; 
        }
      }
    }    



    std::vector< std::string > datesCommon;
    std::vector< unsigned int > indicesCommonHistoricalDates;
    std::vector< unsigned int > indicesCommonFundamentalDates;

    if(validInput){
      //Extract the set of matching dates (within tolerance) between
      //the fundamental and historical data
      validInput = extractDatesOfClosestMatch(
                        datesFundamental,
                        "%Y-%m-%d",
                        datesHistorical,
                        "%Y-%m-%d",
                        maxDayErrorHistoricalData,
                        datesCommon,
                        indicesCommonFundamentalDates,
                        indicesCommonHistoricalDates);

      if(!validInput){
        std::cout << "  Skipping ticker: there is not one matching date between"
                     " the fundamental and historical data sets."
                     << std::endl;
        if(verbose){
          std::cout << "  Skipping: there is not one matching date between"
                     " the fundamental and historical data sets within the"
                     " maximum allowed day error." 
                    << std::endl; 
        }
                     
      }

    }

    //==========================================================================
    //Extract the closest order of the bond yields
    //==========================================================================
    std::vector< std::string > datesBondYields;
    for(auto& el : jsonBondYield["US"]["10y_bond_yield"].items()){
      datesBondYields.push_back(el.key());
    }
    
    
    std::vector< std::string > datesCommonBond;
    std::vector< unsigned int > indicesClosestCommonDates;
    std::vector< unsigned int > indicesClosestBondYieldDates;

    if(validInput){
      //Extract the set of matching dates (within tolerance) between
      //the fundamental, historical, and bond data. The bond data table is
      //quite dense (for the U.S.) so this shouldn't change in size at all.
      validInput = extractDatesOfClosestMatch(
                        datesCommon,
                        "%Y-%m-%d",
                        datesBondYields,
                        "%Y-%m-%d",
                        maxDayErrorBondYieldData,
                        datesCommonBond,
                        indicesClosestCommonDates,
                        indicesClosestBondYieldDates);

      if(datesCommon.size() != datesCommonBond.size()){
        validInput = false;
        std::cout << " Skipping: the bond table is missing entries "
                  << " that are in the set of common dates between "
                  << " the fundamental and historical data" 
                  << std::endl;
        if(verbose){
          std::cout << " Skipping: the bond table is missing entries "
                    << " that are in the set of common dates between "
                    << " the fundamental and historical data" 
                    << std::endl;
        }
                  
      }


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
      double taxRate = 0.;
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
      for( unsigned int indexDate=0;  
                        indexDate < datesCommon.size(); 
                      ++indexDate){

        std::string date = datesCommon[indexDate]; 

        /*
        double taxRateEntry = FinancialAnalysisToolkit::
                                calcTaxRateFromTheTaxProvision(fundamentalData, 
                                            date, 
                                            timePeriod.c_str(), 
                                            false, 
                                            tmpResultName,
                                            termNames,
                                            termValues);
        */
        if(flag_usingTaxTable){
          int year  = std::stoi(date.substr(0,4));
          int yearMin = year-acceptableBackwardsYearErrorForTaxRate;
          taxRate = getTaxRateFromTable(countryISO2, year, yearMin, 
                                        corpWorldTaxTable);
          taxRate = taxRate*0.01;  //The table is in percent                                     
          if(std::isnan(taxRate)){
            taxRate=defaultTaxRate;
          }                              
        }else{
          taxRate = defaultTaxRate;
        }
        
        if(!std::isnan(taxRate)){
          meanTaxRate += taxRate;
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
      int firstMinusLastCommon = 
        calcDifferenceInDaysBetweenTwoDates(
            datesCommon[0],
            "%Y-%m-%d",        
            datesCommon[datesCommon.size()-1],
            "%Y-%m-%d");
      int indexDeltaCommonPrevious = 0;
      if(firstMinusLastCommon > 0){
        indexDeltaCommonPrevious = 1;
      }else{
        indexDeltaCommonPrevious = -1;
      }

      //=======================================================================
      //
      //  Calculate the metrics for every data entry in the file
      //
      //======================================================================= 
      for(int indexDate = 0; 
              indexDate < datesCommon.size();
              ++indexDate){

        std::string date = datesCommon[indexDate];        

        //Get the previous date
        int indexPrevious = indexDate+indexDeltaCommonPrevious;        
        if(indexPrevious >= 0 && indexPrevious < datesCommon.size()){
          previousTimePeriod = datesCommon[indexPrevious];
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
        int indexPastPeriods = indexDate;
        trailingPastPeriods.push_back(datesCommon[indexPastPeriods]);

        for(unsigned int i=0; 
                         i<(numberOfPeriodsToAverageCapitalExpenditures-1); 
                       ++i)
        {
          indexPastPeriods += indexDeltaCommonPrevious;
          if(    indexPastPeriods >= 0 
              && indexPastPeriods < datesCommon.size()){
            trailingPastPeriods.push_back(datesCommon[indexPastPeriods]);
          }
        }

        //======================================================================
        //Evaluate the risk free rate as the yield on a 10 year US bond
        //======================================================================
        int indexBondYield = indicesClosestBondYieldDates[indexDate];
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
        double annualcostOfEquityAsAPercentage = 
          riskFreeRate + equityRiskPremium*beta;

        double costOfEquityAsAPercentage=annualcostOfEquityAsAPercentage;
        if(analyzeQuarterlyData){
          costOfEquityAsAPercentage = costOfEquityAsAPercentage/4.0;
        }        

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
        double defaultSpread = FinancialAnalysisToolkit::
            calcDefaultSpread(fundamentalData,
                              date,
                              timePeriod.c_str(),
                              meanInterestCover,
                              jsonDefaultSpread,
                              appendTermRecord,
                              termNames,
                              termValues);

        std::string parentName = "afterTaxCostOfDebt_";
        

        if(flag_usingTaxTable){
          int year  = std::stoi(date.substr(0,4));
          int yearMin = year-acceptableBackwardsYearErrorForTaxRate;
          taxRate = getTaxRateFromTable(countryISO2, year, yearMin, 
                                        corpWorldTaxTable);
          taxRate = taxRate*0.01;                                        
          if(std::isnan(taxRate)){
            taxRate=meanTaxRate;
          }                    
          if(std::isnan(taxRate)){
            taxRate=defaultTaxRate;
          }          
        }else{
          taxRate = defaultTaxRate;
        }        
        /*
        double taxRate = FinancialAnalysisToolkit::
            calcTaxRateFromTheTaxProvision(fundamentalData,
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
        */
        
                        
        double afterTaxCostOfDebt = (riskFreeRate+defaultSpread)*(1.0-taxRate);
        termNames.push_back("afterTaxCostOfDebt_riskFreeRate");
        termNames.push_back("afterTaxCostOfDebt_defaultSpread");
        termNames.push_back("afterTaxCostOfDebt_taxRate");
        termNames.push_back("afterTaxCostOfDebt");
        
        termValues.push_back(riskFreeRate);
        termValues.push_back(defaultSpread);
        termValues.push_back(taxRate);
        termValues.push_back(afterTaxCostOfDebt);

        //======================================================================
        //Evaluate the current total short and long term debt
        //======================================================================
        double shortLongTermDebtTotal = 
          JsonFunctions::getJsonFloat(
            fundamentalData[FIN][BAL][timePeriod.c_str()][date.c_str()]
                           ["shortLongTermDebtTotal"]);

        if(std::isnan(  shortLongTermDebtTotal) 
                     && replaceNanInShortLongDebtWithLongDebt){
          shortLongTermDebtTotal = 
            JsonFunctions::getJsonFloat(
              fundamentalData[FIN][BAL][timePeriod.c_str()][date.c_str()]
                             ["longTermDebt"]);
        }

        //======================================================================        
        //Evaluate the current market capitalization
        //======================================================================
        double commonStockSharesOutstanding = 
          JsonFunctions::getJsonFloat(
            fundamentalData[FIN][BAL][timePeriod.c_str()][date.c_str()]
                            ["commonStockSharesOutstanding"]);

        unsigned int indexHistoricalData = 
          indicesCommonHistoricalDates[indexDate];   

        std::string closestHistoricalDate= datesHistorical[indexHistoricalData]; 

        double adjustedClose = std::nan("1");
        try{
          adjustedClose = JsonFunctions::getJsonFloat(
                              historicalData[ indexHistoricalData ]["adjusted_close"]);       
        }catch( std::invalid_argument const& ex){
          std::cout << " Historical record (" << closestHistoricalDate << ")"
                    << " is missing an opening share price."
                    << std::endl;
        }

        double marketCapitalization = 
          adjustedClose*commonStockSharesOutstanding;

        //======================================================================        
        //Evaluate a weighted cost of capital
        //======================================================================        

        double costOfCapital = 
          (costOfEquityAsAPercentage*marketCapitalization
          +afterTaxCostOfDebt*shortLongTermDebtTotal)
          /(marketCapitalization+shortLongTermDebtTotal);


        termNames.push_back("costOfCapital_shortLongTermDebtTotal");
        termNames.push_back("costOfCapital_commonStockSharesOutstanding");
        termNames.push_back("costOfCapital_adjustedClose");
        termNames.push_back("costOfCapital_marketCapitalization");
        termNames.push_back("costOfCapital_costOfEquityAsAPercentage");
        termNames.push_back("costOfCapital_afterTaxCostOfDebt");
        termNames.push_back("costOfCapital");

        termValues.push_back(shortLongTermDebtTotal);
        termValues.push_back(commonStockSharesOutstanding);
        termValues.push_back(adjustedClose);
        termValues.push_back(marketCapitalization);
        termValues.push_back(costOfEquityAsAPercentage);
        termValues.push_back(afterTaxCostOfDebt);
        termValues.push_back(costOfCapital);


        //As companies mature they use cheaper forms of capital: debt.
        double costOfCapitalMature = 
          (costOfEquityAsAPercentage*(1-matureFirmFractionOfDebtCapital) 
          + afterTaxCostOfDebt*matureFirmFractionOfDebtCapital);

        if(costOfCapitalMature > costOfCapital){
          costOfCapitalMature=costOfCapital;
        }

        termNames.push_back("costOfCapitalMature_matureFirmDebtCapitalFraction");
        termNames.push_back("costOfCapitalMature_costOfEquityAsAPercentage");
        termNames.push_back("costOfCapitalMature_afterTaxCostOfDebt");
        termNames.push_back("costOfCapitalMature");

        termValues.push_back(matureFirmFractionOfDebtCapital);
        termValues.push_back(costOfEquityAsAPercentage);
        termValues.push_back(afterTaxCostOfDebt);        
        termValues.push_back(costOfCapitalMature);

        //======================================================================
        //Evaluate the metrics
        //  At the moment residual cash flow and the company's valuation are
        //  most of interest. The remaining metrics are useful, however, and so,
        //  have been included.
        //======================================================================        

        double totalStockHolderEquity = JsonFunctions::getJsonFloat(
                fundamentalData[FIN][BAL][timePeriod.c_str()][date.c_str()]
                               ["totalStockholderEquity"]); 
        std::string emptyParentName("");
        double roic = FinancialAnalysisToolkit::
          calcReturnOnInvestedCapital(fundamentalData,
                                      date,
                                      timePeriod.c_str(),
                                      zeroNansInDividendsPaid, 
                                      appendTermRecord, 
                                      emptyParentName,
                                      termNames, 
                                      termValues);

        double roce = FinancialAnalysisToolkit::
          calcReturnOnCapitalDeployed(  fundamentalData,
                                        date,
                                        timePeriod.c_str(), 
                                        appendTermRecord, 
                                        emptyParentName,
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
                                    taxRate,
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
                                     replaceNanInShortLongDebtWithLongDebt,
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
                                 taxRate,
                                 zeroNansInDepreciation,
                                 appendTermRecord,
                                 termNames, 
                                 termValues);

        //Valuation (discounted cash flow)
        double presentValueOfFutureCashFlows = FinancialAnalysisToolkit::
            calcPresentValueOfDiscountedFutureCashFlows(  
              fundamentalData,
              date,
              previousTimePeriod,
              timePeriod.c_str(),
              zeroNansInDividendsPaid,
              zeroNansInDepreciation,
              riskFreeRate,
              costOfCapital,
              costOfCapitalMature,
              taxRate,
              numberOfYearsForTerminalValuation,
              appendTermRecord,
              termNames,
              termValues);

        //Market value (make adjustments as described in Damodaran Ch. 3)
        double cash = JsonFunctions::getJsonFloat(
          fundamentalData[FIN][BAL][timePeriod.c_str()][date.c_str()]["cash"]);

        double netDebt = JsonFunctions::getJsonFloat(
              fundamentalData[FIN][BAL][timePeriod.c_str()][date.c_str()]
                             ["netDebt"]);

        double crossHoldings = std::nan("1");

        double potentialLiabilities = std::nan("1");

        double optionValue = std::nan("1");

        double presentValue = presentValueOfFutureCashFlows
                            + cash 
                            - netDebt;


        //Ratio: price to value
        double priceToValue = marketCapitalization/presentValue;
        if(appendTermRecord){
          termNames.push_back("priceToValue_presentValueOfFutureCashFlows");
          termNames.push_back("priceToValue_cash");
          termNames.push_back("priceToValue_netDebt");
          termNames.push_back("priceToValue_crossHolding_missing");
          termNames.push_back("priceToValue_potentialLiabilities_missing");
          termNames.push_back("priceToValue_stockOptionValuation_missing");
          termNames.push_back("priceToValue_presentValue_approximation");
          termNames.push_back("priceToValue_marketCapitalization");
          termNames.push_back("priceToValue");

          termValues.push_back(presentValueOfFutureCashFlows);
          termValues.push_back(cash);
          termValues.push_back(netDebt);
          termValues.push_back(crossHoldings);
          termValues.push_back(potentialLiabilities);
          termValues.push_back(optionValue);
          termValues.push_back(presentValue);
          termValues.push_back(marketCapitalization);
          termValues.push_back(priceToValue);

        }

        //Residual cash flow to enterprise value
        std::string rFcfToEvLabel = "residualFreeCashFlowToEnterpriseValue_"; 
        double enterpriseValue = FinancialAnalysisToolkit::
            calcEnterpriseValue(fundamentalData, 
                                adjustedClose, 
                                date,
                                timePeriod.c_str(),
                                replaceNanInShortLongDebtWithLongDebt, 
                                appendTermRecord,                                
                                rFcfToEvLabel,
                                termNames,
                                termValues);

        double residualFreeCashFlowToEnterpriseValue = 
          residualCashFlow/enterpriseValue;

        if(appendTermRecord){
          termNames.push_back("residualFreeCashFlowToEnterpriseValue_residualCashFlow");
          termNames.push_back("residualFreeCashFlowToEnterpriseValue");
          termValues.push_back(residualCashFlow);
          termValues.push_back(residualFreeCashFlowToEnterpriseValue);
        }


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
