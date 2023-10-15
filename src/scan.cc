
#include <cstdio>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include <filesystem>

#include "JsonFunctions.h"
#include "FinancialAnalysisToolkit.h"


int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string fundamentalFolder;
  std::string outputFolder;
  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will scan a folder containing fundamental "
    "data (from https://eodhistoricaldata.com/) and will report a summary of "
    "each ticker within the folder along with the results of checks of data "
    "consistency."
    ,' ', "0.0");




    TCLAP::ValueArg<std::string> fundamentalFolderInput("f",
      "fundamental_data_folder_path", 
      "The path to the folder that contains the fundamental data json files from "
      "https://eodhistoricaldata.com/ to analyze",
      true,"","string");

    cmd.add(fundamentalFolderInput);

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","EXCHANGE_CODE", 
      "The exchange code. For example: US",
      true,"","string");

    cmd.add(exchangeCodeInput);  

    TCLAP::ValueArg<std::string> outputFolderInput("o","output_folder_path", 
      "The path to the folder that will contain the output json files "
      "produced by this analysis",
      true,"","string");

    cmd.add(outputFolderInput);

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    fundamentalFolder         = fundamentalFolderInput.getValue();
    exchangeCode              = exchangeCodeInput.getValue();  
    outputFolder              = outputFolderInput.getValue();
    verbose                   = verboseInput.getValue();


    if(verbose){
      std::cout << "  Fundamental Data Folder" << std::endl;
      std::cout << "    " << fundamentalFolder << std::endl;
      
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;


      std::cout << "  Output Folder" << std::endl;
      std::cout << "    " << outputFolder << std::endl;
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
  unsigned int errorCount=0;

  using json = nlohmann::ordered_json;
  json scanResults;

  //Get a list of the json files in the input folder
  for ( const auto & entry 
          : std::filesystem::directory_iterator(fundamentalFolder)){

    //Check to see if the input json file is valid and is for the primary
    //ticker

    bool validInput = false;

    std::string fileName=entry.path().filename();
    size_t lastIndex = fileName.find_last_of(".");
    std::size_t foundExtension = fileName.find(validFileExtension);

    if( foundExtension != std::string::npos ){
        validInput=true;                                                                 
    }

      
    //Process the file if its valid;
    if(validInput){
      //Load the json file
      std::stringstream ss;
      ss << fundamentalFolder << fileName;
      std::string filePathName = ss.str();
      std::ifstream inputJsonFileStream(filePathName.c_str());
      json jsonData = json::parse(inputJsonFileStream);

      std::string name("");
      std::string code("");
      std::string primaryTicker("");      
      std::string exchange("");
      std::string currencyCode("");
      std::string currencySymbol("");
      std::string isin("");
      bool primaryTickerMissing=false;
      bool currencySymbolMissing=false;
      bool isinMissing = false;

    

      JsonFunctions::getJsonString(jsonData[GEN]["Name"], name);
      JsonFunctions::getJsonString(jsonData[GEN]["Code"], code);

      //The spelling of ISIN in the json files is not always consistent      
      JsonFunctions::getJsonString(jsonData[GEN]["ISIN"], isin);
      if(isin.length()==0){
        JsonFunctions::getJsonString(jsonData[GEN]["Isin"], isin);
      }
      if(isin.length()==0){
        JsonFunctions::getJsonString(jsonData[GEN]["isin"], isin);
      }


      JsonFunctions::getJsonString(jsonData[GEN]["PrimaryTicker"], primaryTicker);
      JsonFunctions::getJsonString(jsonData[GEN]["Exchange"], exchange);
      JsonFunctions::getJsonString(jsonData[GEN]["CurrencyCode"], currencyCode);

      bool here=false;
      if(code.compare("PUR")==0){
        here=true;
      }

      auto it= jsonData[FIN][BAL][Y].begin();

      if(it != jsonData[FIN][BAL][Y].end()){
        //A lot of companies do their accounting in USD/EUR, 
        //at least for some years, but are not domiciled in the US nor in Europe
        while(it != jsonData[FIN][BAL][Y].end() && 
                (currencySymbol.length()==0 
                 || currencySymbol.compare("USD")==0
                 || currencySymbol.compare("EUR")==0)){
          std::string quarter = it.key().c_str();
          JsonFunctions::getJsonString(
              jsonData[FIN][BAL][Y][quarter]["currency_symbol"], 
              currencySymbol);
          ++it;
        }

      }else{
        currencySymbolMissing=true;
      }

      if(verbose && currencySymbolMissing){
        ++errorCount;
        std::cout << errorCount  
          << ". Missing: [Financials][Balance_Sheet][currency_symbol] "
          << std::endl << "    "
          <<  fileName << " " << exchange << " " << name <<  std::endl;
      }


      if(currencySymbol.compare(currencyCode) != 0 
          && currencySymbolMissing==false){
        if(primaryTicker.length()==0){
          primaryTickerMissing = true;
        }
      }

      if(isin.length()==0){
        isinMissing = true;
      }

      if(verbose && (primaryTickerMissing || isinMissing)){
        ++errorCount;
        if(primaryTickerMissing){
          std::cout   << errorCount 
                      << ". Missing: PrimaryTicker " << std::endl  
                      << "    " 
                      << fileName << " " << exchange << " " << name << std::endl;
        }
        //if(isinMissing){
        //  std::cout   << errorCount 
        //              << ". Missing: ISIN " << std::endl  
        //              << "    " 
        //              << fileName << " " << exchange << " " << name << std::endl;
        //}
      }

      json tickerEntry = json::object( 
                        { 
                          {"Code", code},
                          {"Name", name},
                          {"Exchange", exchange},
                          {"ISIN",isin},
                          {"PrimaryTicker", primaryTicker},
                          {"CurrencyCode", currencyCode},
                          {"currency_symbol", currencySymbol},
                          {"MissingISIN", isinMissing},
                          {"MissingPrimaryTicker", primaryTickerMissing},
                          {"MissingCurrencySymbol", currencySymbolMissing},
                        }
                      );

      scanResults[code]= tickerEntry;

    }

    ++count;
  }

  std::string outputFilePath(outputFolder);
  std::string outputFileName(exchangeCode);

  outputFileName.append(".");
  outputFileName.append("scan");
  outputFileName.append(".json");
  
  //Update the extension 
  outputFilePath.append(outputFileName);

  std::ofstream outputFileStream(outputFilePath,
      std::ios_base::trunc | std::ios_base::out);
  outputFileStream << scanResults;
  outputFileStream.close();

  std::cout << "success" << std::endl;

  return 0;
}
