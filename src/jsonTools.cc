//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT


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
  std::string iso3166FileName;
  std::string mergeByCountryFileName;
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

    TCLAP::ValueArg<std::string> iso3166FileNameInput("x","iso_3166_file",
      "The full file path to the json file that contains a list of all"
      "country and region names. Please use "
      "data/ISO-3166-Countries-with-Regional-Codes or clone the list from "
      "https://github.com/lukes/ISO-3166-Countries-with-Regional-Codes/"
      " Note: this option cannot be used with the -p"
      "option.",false,"","string");
    cmd.add(iso3166FileNameInput);  

    TCLAP::ValueArg<std::string> mergeByCountryFileNameInput("m","merge_file_name",
      "The full file path to the json file that contains extra fields to append "
      "to the input file, where entries are matched by country name."
      " Note: this option cannot be used with the -p"
      "option.",false,"","string");
    cmd.add(mergeByCountryFileNameInput);   


    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    inputJsonFileName         = inputJsonFileNameInput.getValue();
    outputJsonFileName        = outputJsonFileNameInput.getValue();
    splitPrimaryTicker        = splitPrimaryTickerInput.getValue();
    iso3166FileName           = iso3166FileNameInput.getValue();
    mergeByCountryFileName    = mergeByCountryFileNameInput.getValue();
    verbose                   = verboseInput.getValue();

    int optionCheck=0;
    if(splitPrimaryTicker){
      ++optionCheck;
    }
    if(iso3166FileName.length()>0){
      ++optionCheck;
    }
    if(mergeByCountryFileName.length()>0){
      ++optionCheck;
    }


    if(optionCheck>1 || optionCheck==0){
      std::cout << "One of these options must be used: -p, -x, or -m"
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
      std::cout << "    " << iso3166FileName << std::endl;

      std::cout << "   Merge entries in this json file with the input (by country)" 
                << std::endl;
      std::cout << "    " << mergeByCountryFileName << std::endl;


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

  //============================================================================
  // Split the primary ticker
  //============================================================================  
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

  //============================================================================
  // Add the ISO2 and ISO3 country codes
  //============================================================================  
  if(iso3166FileName.length()>0){
    std::ifstream iso3166FileStream(iso3166FileName);
    json iso3166Data = json::parse(iso3166FileStream);   

    for(auto& entry: jsonData){
      json jsonDataEntry = nlohmann::ordered_json::object();

      for(auto& el: entry.items()){
        if(el.key().compare("Country")==0){
          //Go through the list of exchanges
          std::string country = el.value();
          bool found=false;
          std::string countryISO2, countryISO3;
          for(auto& iso3166Entry: iso3166Data){
            for(auto& iso3166Fields: iso3166Entry.items()){
              if(iso3166Fields.key().compare("name")==0){
                std::string iso3166Country = iso3166Fields.value();
                if( country.compare(iso3166Country) == 0){
                  JsonFunctions::getJsonString(iso3166Entry["alpha-2"],countryISO2);
                  JsonFunctions::getJsonString(iso3166Entry["alpha-3"],countryISO3);
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

  //============================================================================
  // Merge the extry fields with the input 
  //============================================================================  
  if(mergeByCountryFileName.length()>0){

    std::ifstream mergeFileStream(mergeByCountryFileName);
    json mergeData = json::parse(mergeFileStream);       

    json jsonDataMergeDefault = nlohmann::ordered_json::object();

    //Copy over an empty version of the extra merge fields, but ignore the 
    //country entry
    for(auto& elMerge : mergeData.at(0).items() ){
      if(elMerge.key().compare("Country") != 0){
        if(elMerge.value().is_string()){
          jsonDataMergeDefault.push_back({elMerge.key(),""});
        }else{
          jsonDataMergeDefault.push_back({elMerge.key(),std::nan("1")});
        }
      }
    }

    for(auto& entry: jsonData.items()){

      json jsonDataEntry = nlohmann::ordered_json::object();
      std::string country;
      JsonFunctions::getJsonString(entry.value()["Country"],country);

      //Copy the existing fields over.
      for(auto& elEntry : entry.value().items()){
        jsonDataEntry.push_back({elEntry.key(), elEntry.value()});
      }

      bool matchFound = false;
      //If a match exists, copy over the contents
      for(auto& entryMerge : mergeData.items()){
        std::string mergeCountry;
        JsonFunctions::getJsonString(entryMerge.value()["Country"],mergeCountry);

        if(country.compare(mergeCountry)==0){

          //Copy over an empty set of fields to append.
          matchFound=true;

          for(auto& elMerge : entryMerge.value().items()){
            if(elMerge.key().compare("Country") != 0){
              jsonDataEntry.push_back({elMerge.key(), elMerge.value()});              
            }
          }
        }

      }

      if(matchFound){
        jsonDataUpd.push_back(jsonDataEntry);
      }else{
        for(auto &el : jsonDataMergeDefault.items()){
          jsonDataEntry.push_back({el.key(),el.value()});
        }
        jsonDataUpd.push_back(jsonDataEntry);
      }

    
    }
    

  }


  std::ofstream outputFileStream(outputJsonFileName,
      std::ios_base::trunc | std::ios_base::out);
  outputFileStream << jsonDataUpd;
  outputFileStream.close();

  std::cout << "success" << std::endl;

  return 0;
}
