
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
  double annualCostOfEquityAsAPercentage;
  double costOfEquityAsAPercentage;
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

    TCLAP::ValueArg<double> annualCostOfEquityAsAPercentageInput("c",
      "COST_OF_EQUITY_PERCENTAGE", 
      "The annual cost of equity as a percentage. Default value of 0.10 taken "
      "from Ch. 12 of Lev and Gu.",
      false,0.10,"double");

    cmd.add(annualCostOfEquityAsAPercentageInput);  

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

    annualCostOfEquityAsAPercentage 
      = annualCostOfEquityAsAPercentageInput.getValue();

    numberOfYearsToAverageCapitalExpenditures 
      = numberOfYearsToAverageCapitalExpendituresInput.getValue();              

    verbose             = verboseInput.getValue();

    if(analyzeQuarterlyData){
      timePeriod                  = Q;
      costOfEquityAsAPercentage   = annualCostOfEquityAsAPercentage/4.0;
      numberOfPeriodsToAverageCapitalExpenditures = 
        numberOfYearsToAverageCapitalExpenditures*4;

    }else{
      timePeriod                  = Y;
      costOfEquityAsAPercentage   = annualCostOfEquityAsAPercentage;
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

      std::cout << "  Analyze Quaterly Data" << std::endl;
      std::cout << "    " << analyzeQuarterlyData << std::endl;

      std::cout << "  Annual cost of equity" << std::endl;
      std::cout << "    " << annualCostOfEquityAsAPercentage << std::endl;

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

  //Get a list of the json files in the input folder
  for ( const auto & entry 
          : std::filesystem::directory_iterator(fundamentalFolder)){

    //Check to see if the input json file is valid and is for the primary
    //ticker

    bool validInput = true;

    std::string fileName=entry.path().filename();
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

      
    //Process the file if its valid;
    if(validInput){
      //Load the json file
      std::stringstream ss;
      ss << fundamentalFolder << fileName;
      std::string filePathName = ss.str();
      using json = nlohmann::ordered_json;
      std::ifstream inputJsonFileStream(filePathName.c_str());
      json jsonData = json::parse(inputJsonFileStream);

      json analysis;

      std::vector< std::string > entryDates;

      //By default this will analyze annual data
      for(auto& el : jsonData[FIN][BAL][timePeriod].items()){
        //entryDates.push_back(el.key());
        entryDates.insert(entryDates.begin(),el.key());
      }
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

      for( auto& it : entryDates){
        std::string date = it;        



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
        
        double totalStockHolderEquity = 
        JsonFunctions::getJsonFloat(jsonData[FIN][BAL][timePeriod.c_str()][it.c_str()]
                      ["totalStockholderEquity"]); 


        double roic = FinancialAnalysisToolkit::
          calcReturnOnInvestedCapital(jsonData,it,timePeriod.c_str());
        double roce = FinancialAnalysisToolkit::
          calcReturnOnCapitalDeployed(jsonData,it,timePeriod.c_str());

        double grossMargin = FinancialAnalysisToolkit::
          calcGrossMargin(jsonData,it,timePeriod.c_str());
        double operatingMargin = FinancialAnalysisToolkit::
          calcOperatingMargin(jsonData,it,timePeriod.c_str());          
        double cashConversion = FinancialAnalysisToolkit::
          calcCashConversionRatio(jsonData,it,timePeriod.c_str());

        bool zeroNanInShortTermDebt=true;
        double debtToCapital = FinancialAnalysisToolkit::
          calcDebtToCapitalizationRatio(jsonData,it,timePeriod.c_str(),
                                              zeroNanInShortTermDebt);   
        double interestCover = FinancialAnalysisToolkit::
          calcInterestCover(jsonData,it,timePeriod.c_str()); 

        double ownersEarnings = FinancialAnalysisToolkit::
          calcOwnersEarnings(jsonData,it,timePeriod.c_str());                 

        double residualCashFlow = std::nan("1");
        if(trailingPastPeriods.size() == numberOfPeriodsToAverageCapitalExpenditures){
          bool zeroNansInResearchAndDevelopment=true;
          residualCashFlow = FinancialAnalysisToolkit::
            calcResidualCashFlow( jsonData,
                                  it,
                                  timePeriod.c_str(),
                                  costOfEquityAsAPercentage,
                                  trailingPastPeriods,
                                  zeroNansInResearchAndDevelopment);
        }

        double freeCashFlowToEquity=std::nan("1");
        if(previousTimePeriod.length()>0){
          freeCashFlowToEquity = 
            FinancialAnalysisToolkit::calcFreeCashFlowToEquity(jsonData, 
                                     it,
                                     previousTimePeriod,
                                     timePeriod.c_str());
        }

        //it.c_str(), 
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
                            {"totalStockHolderEquity",totalStockHolderEquity}
                          }
                        );

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
