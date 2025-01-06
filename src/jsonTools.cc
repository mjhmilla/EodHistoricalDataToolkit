
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
  std::string exchangeJsonFileName;
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

    TCLAP::SwitchArg splitPrimaryTickerInput("p","split_primary_ticker",
      "Will update the PrimaryTicker to be PrimaryTicker and PrimaryExchange. "
      "As an example a PrimaryTicker of OCSL.US would turn into a PrimaryTicker "
      "of OCSL and a PrimaryExchange of US. This cannot be used with the -x "
      "option.", false);
    cmd.add(splitPrimaryTickerInput);    

    TCLAP::ValueArg<std::string> exchangeJsonFileNameInput("x","exchange_json_file",
      "The full file path to the json file that contains a list of all"
      "exhanges covered in the data set. This cannot be used with the -p"
      "option.",false,"","string");
    cmd.add(exchangeJsonFileNameInput);   


    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    inputJsonFileName         = inputJsonFileNameInput.getValue();
    outputJsonFileName        = outputJsonFileNameInput.getValue();
    splitPrimaryTicker        = splitPrimaryTickerInput.getValue();
    exchangeJsonFileName      = exchangeJsonFileNameInput.getValue();
    verbose                   = verboseInput.getValue();

    if(splitPrimaryTicker && exchangeJsonFileName.length()>0){
      std::cout << "Use either the -p option or the -x option but not both"
                << std::endl;
      std::abort();                
    }


    if(verbose){
      std::cout << "  Input Json File" << std::endl;
      std::cout << "    " << inputJsonFileName << std::endl;
      
      std::cout << "  Output Json File" << std::endl;
      std::cout << "    " << outputJsonFileName << std::endl;
  
      std::cout << "  Split primary ticker?" << std::endl;
      std::cout << "    " << splitPrimaryTicker << std::endl;

      std::cout << "  Correct country names in list to be consistent with this file?" 
                << std::endl;
      std::cout << "    " << exchangeJsonFileName << std::endl;

    }

  } catch (TCLAP::ArgException &e)  // catch exceptions
	{ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }



  using json = nlohmann::ordered_json;

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

  if(exchangeJsonFileName.length()>0){
    std::ifstream exchangeFileStream(exchangeJsonFileName);
    json exchangeData = json::parse(exchangeFileStream);   

    for(auto& entry: jsonData){
      json jsonDataEntry = nlohmann::ordered_json::object();

      for(auto& el: entry.items()){
        if(el.key().compare("Country")==0){
          //Go through the list of exchanges
          std::string country = el.value();
          bool found=false;
          std::string countryISO2, countryISO3;
          for(auto& exEntry: exchangeData){
            for(auto& exFields: exEntry.items()){
              if(exFields.key().compare("Country")==0){
                std::string exCountry = exFields.value();
                if( country.compare(exCountry) == 0){
                  JsonFunctions::getJsonString(exEntry["CountryISO2"],countryISO2);
                  JsonFunctions::getJsonString(exEntry["CountryISO3"],countryISO3);
                  found = true;
                  break;
                }
              }
            }
            if(found){
              break;
            }
          }
          if(found){
            jsonDataEntry.push_back({el.key(), country});
            jsonDataEntry.push_back({"CountryISO2", countryISO2});
            jsonDataEntry.push_back({"CountryISO3", countryISO3});
          }else{
            jsonDataEntry.push_back({el.key(), country});
            jsonDataEntry.push_back({"CountryISO2", ""});
            jsonDataEntry.push_back({"CountryISO3", ""});            
          }
          

        }else{
          jsonDataEntry.push_back({el.key(),el.value()});
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
