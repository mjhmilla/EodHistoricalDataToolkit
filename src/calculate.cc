
#include <cstdio>
#include <fstream>
#include <string>

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

    TCLAP::ValueArg<std::string> analyseFolderOutput("o","output_folder_path", 
      "The path to the folder that will contain the output json files "
      "produced by this analysis",
      true,"","string");

    cmd.add(analyseFolderOutput);

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    fundamentalFolder   = fundamentalFolderInput.getValue();
    historicalFolder    = historicalFolderInput.getValue();
    exchangeCode        = exchangeCodeInput.getValue();    
    analyseFolder       = analyseFolderOutput.getValue();
    verbose             = verboseInput.getValue();

    if(verbose){
      std::cout << "  Fundamental Data Folder" << std::endl;
      std::cout << "    " << fundamentalFolder << std::endl;

      std::cout << "  Historical Data Folder" << std::endl;
      std::cout << "    " << historicalFolder << std::endl;
      
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

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

    bool validInput = false;

    std::string fileName=entry.path().filename();
    size_t lastIndex = fileName.find_last_of(".");
    std::string tickerName = fileName.substr(0,lastIndex);
    std::size_t foundExtension = fileName.find(validFileExtension);

    if( foundExtension != std::string::npos ){
        validInput=true;
        std::string updTickerName = tickerName;
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
        if(updTickerName.compare(tickerName) != 0){
          fileName = updTickerName;
          fileName.append(".json");
          tickerName = updTickerName;
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

      for(auto& el : jsonData[FIN][BAL][Q].items()){
        entryDates.push_back(el.key());
      }




      for( auto& it : entryDates){
        std::string date = it;

        double roce = FinancialAnalysisToolkit::
                        calcReturnOnCapitalDeployed(jsonData,it,Q);
        double grossMargin = FinancialAnalysisToolkit::
                        calcGrossMargin(jsonData,it,Q);
        double operatingMargin = FinancialAnalysisToolkit::
                        calcOperatingMargin(jsonData,it,Q);
        double cashConversion = FinancialAnalysisToolkit::
                        calcCashConversionRatio(jsonData,it,Q);
        double debtToCapital = FinancialAnalysisToolkit::
                        calcDebtToCapitalizationRatio(jsonData,it,Q);   
        double interestCover = FinancialAnalysisToolkit::
                        calcInterestCover(jsonData,it,Q);  

        //it.c_str(), 
        json analysisEntry = json::object( 
                          { 
                            {"returnOnCapitalDeployed", roce},
                            {"grossMargin",grossMargin},
                            {"operatingMargin",operatingMargin},
                            {"cashConversionRatio",cashConversion},
                            {"debtToCapitalRatio",debtToCapital},
                            {"interestCover",interestCover}
                          }
                        );

        analysis[it]= analysisEntry;
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
