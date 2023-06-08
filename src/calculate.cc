
#include <cstdio>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include <filesystem>

#include "FinancialAnalysisToolkit.h"

unsigned int COLUMN_WIDTH = 30;

int main (int argc, char* argv[]) {

  std::string inputFolder;
  std::string outputFolder;
  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will analyze the json files "
    "(from https://eodhistoricaldata.com/) in an input directory and "
    "write the results of the analysis to a json file in an output directory"
    ,' ', "0.0");


    TCLAP::ValueArg<std::string> inputFolderInput("i","input_folder_path", 
      "The path to the folder that contains the json files from "
      "https://eodhistoricaldata.com/ to analyze",
      true,"","string");

    cmd.add(inputFolderInput);

    TCLAP::ValueArg<std::string> outputFolderInput("o","output_folder_path", 
      "The path to the folder that will contain the output json files "
      "produced by this analysis",
      true,"","string");

    cmd.add(outputFolderInput);

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    inputFolder   = inputFolderInput.getValue();
    outputFolder  = outputFolderInput.getValue();
    verbose       = verboseInput.getValue();

    if(verbose){
      std::cout << "  Input Folder" << std::endl;
      std::cout << "    " << inputFolder << std::endl;

      std::cout << "  Output Folder" << std::endl;
      std::cout << "    " << outputFolder << std::endl;
    }
  } catch (TCLAP::ArgException &e)  // catch exceptions
	{ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }

  auto startingDirectory = std::filesystem::current_path();
  std::filesystem::current_path(inputFolder);


  //Get a list of the json files in the input folder

  for ( const auto & entry : std::filesystem::directory_iterator(inputFolder)){

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
                        calcReturnOnCapitalDeployed(jsonData,it);
        double roa = FinancialAnalysisToolkit::
                        calcReturnOnAssets(jsonData,it);
        double roit = FinancialAnalysisToolkit::
                        calcReturnOnInvestedCapital(jsonData,it);

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




      std::string outputFilePath(outputFolder);
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
