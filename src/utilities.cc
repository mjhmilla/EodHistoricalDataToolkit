
#include <cstdio>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include <filesystem>

#include "JsonFunctions.h"


int main (int argc, char* argv[]) {

  std::string inputJsonFileName;
  std::string outputJsonFileName;
  bool splitPrimaryTicker;
  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command performs a number of small rarely needed "
    "utility operations on json files."
    ,' ', "0.0");


    TCLAP::ValueArg<std::string> inputJsonFileNameInput("i",
      "input_json_file", 
      "The path and file name of the input json file to process",
      true,"","string");

    cmd.add(inputJsonFileNameInput);

    TCLAP::ValueArg<std::string> outputJsonFileNameInput("o",
      "output_json_file", 
      "The path and file name of the output json file that will be written",
      true,"","string");

    cmd.add(outputJsonFileNameInput);

    TCLAP::SwitchArg splitPrimaryTickerInput("s","split_primary_ticker",
      "Will update the PrimaryTicker to be PrimaryTicker and PrimaryExchange. "
      "As an example a PrimaryTicker of OCSL.US would turn into a PrimaryTicker "
      "of OCSL and a PrimaryExchange of US", false);
    cmd.add(splitPrimaryTickerInput);    


    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    inputJsonFileName         = inputJsonFileNameInput.getValue();
    outputJsonFileName        = outputJsonFileNameInput.getValue();
    splitPrimaryTicker        = splitPrimaryTickerInput.getValue();
    verbose                   = verboseInput.getValue();


    if(verbose){
      std::cout << "  Input Json File" << std::endl;
      std::cout << "    " << inputJsonFileName << std::endl;
      
      std::cout << "  Output Json File" << std::endl;
      std::cout << "    " << outputJsonFileName << std::endl;
  
      std::cout << "  Split primary ticker?" << std::endl;
      std::cout << "    " << splitPrimaryTicker << std::endl;
    }
  } catch (TCLAP::ArgException &e)  // catch exceptions
	{ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }



  using json = nlohmann::ordered_json;
  json inputFile;

  std::ifstream inputFileStream(inputJsonFileName);
  json jsonData = json::parse(inputFileStream);        
  json jsonDataUpd;

  if(splitPrimaryTicker){
    for(auto& entry: jsonData){
      json jsonDataEntry;

      for(auto& el: entry.items()){
        if(el.key().compare("PrimaryTicker")==0){
          std::string primaryTicker = el.value();
          size_t idx = primaryTicker.find_last_of(".");
          std::string ticker = primaryTicker.substr(0UL,idx);
          std::string exchange = primaryTicker.substr(idx+1,primaryTicker.length()-1);
          
          jsonDataEntry[el.key()]=ticker;
          jsonDataEntry["PrimaryExchange"]=exchange;

        }else{
          jsonDataEntry[el.key()] = el.value();          
        }

      }

      jsonDataUpd.push_back(jsonDataEntry);

    }
  }


  std::ofstream outputFileStream(outputJsonFileName,
      std::ios_base::trunc | std::ios_base::out);
  outputFileStream << jsonDataUpd;
  outputFileStream.close();

  std::cout << "success" << std::endl;

  return 0;
}
