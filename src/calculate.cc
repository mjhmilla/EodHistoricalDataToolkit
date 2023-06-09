
#include <cstdio>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include <filesystem>

#include "FinancialAnalysisToolkit.h"

unsigned int COLUMN_WIDTH = 30;

int main (int argc, char* argv[]) {

  std::string fundamentalFolder;
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

    TCLAP::ValueArg<std::string> eodFolderInput("f",
      "eod_data_folder_path", 
      "The path to the folder that contains the end-of-day data csv files from "
      "https://eodhistoricaldata.com/ to analyze",
      true,"","string");

    cmd.add(eodFolderInput);

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
    eodFolder           = fundamentalFolderInput.getValue();
    analyseFolder       = analyseFolderOutput.getValue();
    verbose             = verboseInput.getValue();

    if(verbose){
      std::cout << "  Fundamental Data Folder" << std::endl;
      std::cout << "    " << fundamentalFolder << std::endl;

      std::cout << "  End-Of-Day Data Folder" << std::endl;
      std::cout << "    " << eodFolder << std::endl;

      std::cout << "  Analyse Folder" << std::endl;
      std::cout << "    " << analyseFolder << std::endl;
    }
  } catch (TCLAP::ArgException &e)  // catch exceptions
	{ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }

  auto startingDirectory = std::filesystem::current_path();
  std::filesystem::current_path(fundamentalFolder);


  //Get a list of the json files in the input folder

  for ( const auto & entry 
          : std::filesystem::directory_iterator(fundamentalFolder)){

    //Check to see if the input json file is valid
    bool validInput = false;
    if(entry.path().has_extension()){
      if( entry.path().extension().compare("json")){
        validInput=true;
        if(verbose){
          std::cout << "    " << entry.path() << std::endl;
        }
      }      
    }

    //Process the file if its valid;
    if(validInput){
      //Load the json file
      using json = nlohmann::ordered_json;
      std::ifstream inputJsonFileStream(entry.path().c_str());
      json jsonData = json::parse(inputJsonFileStream);

      json analysis = json::array();

      std::vector< std::string > entryDates;

      for(auto& el : jsonData[FIN][BAL][Q].items()){
        entryDates.push_back(el.key());
      }




      for( auto& it : entryDates){
        std::string date = it;

        double roce = FinancialAnalysisToolkit::
                        calcReturnOnCapitalDeployed(jsonData,it,Q);
        double roa = FinancialAnalysisToolkit::
                        calcReturnOnAssets(jsonData,it,Q);
        double roit = FinancialAnalysisToolkit::
                        calcReturnOnInvestedCapital(jsonData,it,Q);

        if(!std::isnan(roce)){
          json roce = {it.c_str(),{ "roce", roce}};
          analysis.push_back(roce);
        }

      }



      //Evaluate the ROCE
      // I'm going to use a conservative definition:
      // netIncome / (totalStockholderEquity + longTermDebt)

      //Equity growth

      //Earnings growth

      //Sales growth

      //free-cash-flow growth




      std::string outputFilePath(analyseFolder);
      std::string outputFileName(entry.path().filename().c_str());
      
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
