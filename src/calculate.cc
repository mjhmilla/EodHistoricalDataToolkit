
#include <cstdio>
#include <fstream>
#include <string>
#include <cmath>

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

  double riskFreeRate;
  double equityRiskPremium;
  double defaultBeta;

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

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","EXCHANGE_CODE", 
      "The exchange code. For example: US",
      false,"","string");

    cmd.add(exchangeCodeInput);  


    TCLAP::ValueArg<std::string> defaultSpreadJsonFileInput("d",
      "DEFAULT_SPREAD_JSON_FILE_PATH", 
      "The path to the json file that contains a table relating interest"
      " coverage to default spread",false,"","string");

    cmd.add(defaultSpreadJsonFileInput);  

    TCLAP::ValueArg<double> defaultTaxRateInput("t",
      "DEFAULT_TAX_RATE", 
      "The tax rate used if the tax rate cannot be calculated, or averaged, from"
      " the data",
      false,0.30,"double");

    cmd.add(defaultTaxRateInput);  


    TCLAP::ValueArg<double> riskFreeRateInput("r",
      "RISK_FREE_RATE", 
      "The risk free rate of return, which is often set to the return on "
      "a 10 year or 30 year bond as noted from Ch. 3 of Lev and Gu.",
      false,0.025,"double");

    cmd.add(riskFreeRateInput);  

    TCLAP::ValueArg<double> equityRiskPremiumInput("c",
      "EQUITY_RISK_PREMIUM", 
      "The extra return that the stock should return given its risk. Often this"
      " is set to the historical incremental rate of return provided by the "
      " stock market relative to the bond market. In the U.S. this is somewhere"
      " around 4 percent between 1928 and 2010 as noted in Ch. 3 of Lev and Gu."
      " Given the high inflation rate, it might be wise to demande more than 0.05.",
      false,0.05,"double");

    cmd.add(equityRiskPremiumInput);  

    TCLAP::ValueArg<double> defaultBetaInput("b",
      "DEFAULT_BETA", 
      "The default beta value to use when one is not reported",
      false,1.0,"double");

    cmd.add(defaultBetaInput);  

    //numberOfYearsToAverageCapitalExpenditures
    TCLAP::ValueArg<int> numberOfYearsToAverageCapitalExpendituresInput("n",
      "NUMBER_OF_YEARS_TO_AVERAGE_CAPITAL_EXPENDITURES", 
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
    historicalFolder      = historicalFolderInput.getValue();
    exchangeCode          = exchangeCodeInput.getValue();    
    analyseFolder         = analyseFolderOutput.getValue();
    analyzeQuarterlyData  = quarterlyAnalysisInput.getValue();

    defaultTaxRate      = defaultTaxRateInput.getValue();
    riskFreeRate        = riskFreeRateInput.getValue();
    defaultBeta         = defaultBetaInput.getValue();
    equityRiskPremium   = equityRiskPremiumInput.getValue();


    defaultSpreadJsonFile = defaultSpreadJsonFileInput.getValue();

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

      std::cout << "  Default Spread Json File" << std::endl;
      std::cout << "    " << defaultSpreadJsonFile << std::endl;

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

  std::string validFileExtension = exchangeCode;
  validFileExtension.append(".json");

  auto startingDirectory = std::filesystem::current_path();
  std::filesystem::current_path(fundamentalFolder);

  unsigned int count=0;

  using json = nlohmann::ordered_json;    

  std::ifstream defaultSpreadFileStream(defaultSpreadJsonFile.c_str());
  json jsonDefaultSpread = nlohmann::ordered_json::parse(defaultSpreadFileStream);

  for(auto &row: jsonDefaultSpread.items()){
    for(auto &ele: row.value()){
      std::cout << ele << " ";
    }
    std::cout << std::endl;
  }

  std::cout << jsonDefaultSpread.at(1).at(2);

  bool zeroNanInShortTermDebt=true;
  bool zeroNansInResearchAndDevelopment=true;
  bool zeroNansInDividendsPaid = true;
  bool zeroNansInDepreciation = true;
  bool appendTermRecord=true;
  std::vector< std::string >  termNames;
  std::vector< double >       termValues;  

  bool loadSingleTicker=true;
  std::string singleTicker = "MMM.STU.json";


  //Get a list of the json files in the input folder
  for ( const auto & entry 
          : std::filesystem::directory_iterator(fundamentalFolder)){

    //Check to see if the input json file is valid and is for the primary
    //ticker

    bool validInput = true;

    std::string fileName=entry.path().filename();
    if(loadSingleTicker){
      fileName  = singleTicker;
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

    //Try to load the file

    nlohmann::ordered_json jsonData;
    if(validInput){
      try{
        //Load the json file
        std::stringstream ss;
        ss << fundamentalFolder << fileName;
        std::string filePathName = ss.str();

        std::ifstream inputJsonFileStream(filePathName.c_str());
        jsonData = nlohmann::ordered_json::parse(inputJsonFileStream);
      }catch(const nlohmann::json::parse_error& e){
        std::cout << e.what() << std::endl;
        validInput=false;
      }
    }
    //Process the file if its valid;
    nlohmann::ordered_json analysis;
    std::vector< std::string > entryDates;
    if(validInput){
      //By default this will analyze annual data
      for(auto& el : jsonData[FIN][BAL][timePeriod].items()){
        //entryDates.push_back(el.key());
        entryDates.insert(entryDates.begin(),el.key());
      }
      if(entryDates.size()==0){
        std::cout << "  File contains no date entries" << std::endl;
        validInput=false;
      }
    }

    if(validInput){
      //Check that the dates are ordered from oldest to newest
      std::istringstream timeStreamFirst(entryDates.front());
      std::istringstream timeStreamLast(entryDates.back());
      timeStreamFirst.imbue(std::locale("en_US.UTF-8"));
      timeStreamLast.imbue(std::locale("en_US.UTF-8"));
      std::tm firstTime = {};
      std::tm lastTime  = {};
      timeStreamFirst >> std::get_time(&firstTime,"%Y-%m-%d");
      timeStreamLast  >> std::get_time(&lastTime,"%Y-%m-%d");
      if(timeStreamFirst.fail() || timeStreamLast.fail()){
        std::cerr << "Error: converting date strings to double values failed"
                  << std::endl;
        std::abort();
      }
      std::mktime(&firstTime);
      std::mktime(&lastTime);
      //Make sure the first time is smaller than the last time
      if(firstTime.tm_year > lastTime.tm_year){
        std::cerr << "Error: time entries are in the opposite order than expected"
                  << std::endl;
        std::abort();
      }


      std::vector< std::string > trailingPastPeriods;
      std::string previousTimePeriod("");

      unsigned int entryCount = 0;

      //calculate the average tax rate
      std::vector< std::string > tmpNames;
      std::vector< double > tempValues;
      std::string tmpResultName("");
      unsigned int taxRateEntryCount = 0;
      double meanTaxRate = 0.;

      double beta = JsonFunctions::getJsonFloat(jsonData[GEN][TECH]["Beta"]);
      if(std::isnan(beta)){
        beta=defaultBeta;
      }
      double annualCostOfEquityAsAPercentage = 
          riskFreeRate + equityRiskPremium*beta;

      double costOfEquityAsAPercentage=annualCostOfEquityAsAPercentage;
      if(analyzeQuarterlyData){
        costOfEquityAsAPercentage = costOfEquityAsAPercentage/4.0;
      }

      for( auto& it : entryDates){
        std::string date = it;           
        double taxRateEntry = FinancialAnalysisToolkit::calcTaxRate(
          jsonData,date,timePeriod.c_str(), false,tmpResultName,termNames,
          termValues);
        if(!std::isnan(taxRateEntry)){
          meanTaxRate += taxRateEntry;
          ++taxRateEntryCount;
        }
      }     
      meanTaxRate = meanTaxRate / static_cast<double>(taxRateEntryCount);
      if(std::isnan(meanTaxRate)){
        meanTaxRate=defaultTaxRate;
      }


      for( auto& it : entryDates){
        std::string date = it;        


        termNames.clear();
        termValues.clear();

        termNames.push_back("costOfEquityAsAPercentage_riskFreeRate");
        termNames.push_back("costOfEquityAsAPercentage_equityRiskPremium");
        termNames.push_back("costOfEquityAsAPercentage_beta");
        termNames.push_back("costOfEquityAsAPercentage");

        termValues.push_back(riskFreeRate);
        termValues.push_back(equityRiskPremium);
        termValues.push_back(beta);
        termValues.push_back(costOfEquityAsAPercentage);


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
        
        double totalStockHolderEquity = JsonFunctions::getJsonFloat(
          jsonData[FIN][BAL][timePeriod.c_str()][it.c_str()]["totalStockholderEquity"]); 


        double roic = FinancialAnalysisToolkit::
          calcReturnOnInvestedCapital(jsonData,
                                      it,
                                      timePeriod.c_str(),
                                      zeroNansInDividendsPaid, 
                                      appendTermRecord, 
                                      termNames, 
                                      termValues);

        double roce = FinancialAnalysisToolkit::
          calcReturnOnCapitalDeployed(  jsonData,
                                        it,
                                        timePeriod.c_str(), 
                                        appendTermRecord, 
                                        termNames, 
                                        termValues);

        double grossMargin = FinancialAnalysisToolkit::
          calcGrossMargin(  jsonData,
                            it,
                            timePeriod.c_str(),
                            appendTermRecord,
                            termNames,
                            termValues);

        double operatingMargin = FinancialAnalysisToolkit::
          calcOperatingMargin(  jsonData,
                                it,
                                timePeriod.c_str(), 
                                appendTermRecord,
                                termNames,
                                termValues);          

        double cashConversion = FinancialAnalysisToolkit::
          calcCashConversionRatio(  jsonData,
                                    it,
                                    timePeriod.c_str(), 
                                    meanTaxRate,
                                    appendTermRecord,
                                    termNames,
                                    termValues);

        double debtToCapital = FinancialAnalysisToolkit::
          calcDebtToCapitalizationRatio(  jsonData,
                                          it,
                                          timePeriod.c_str(),
                                          zeroNanInShortTermDebt,
                                          appendTermRecord,
                                          termNames,
                                          termValues);

        double interestCover = FinancialAnalysisToolkit::
          calcInterestCover(  jsonData,
                              it,
                              timePeriod.c_str(),
                              appendTermRecord,
                              termNames,
                              termValues); 

        double ownersEarnings = FinancialAnalysisToolkit::
          calcOwnersEarnings( jsonData, 
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
            calcResidualCashFlow( jsonData,
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
            calcFreeCashFlowToEquity(jsonData, 
                                     it,
                                     previousTimePeriod,
                                     timePeriod.c_str(),
                                     appendTermRecord,
                                     termNames,
                                     termValues);
        }

        double freeCashFlowToFirm=std::nan("1");
        freeCashFlowToFirm = FinancialAnalysisToolkit::
          calcFreeCashFlowToFirm(jsonData, 
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
  }




  //For each file in the list
    //Open it
    //Get jsonData["General"]["PrimaryTicker"] and use this to generate an output file name
    //Create a new json file to contain the analysis
    //Call each analysis function individually to analyze the json file and
    //  to write the results of the analysis into the output json file
    //Write the analysis json file 

  return 0;
}
