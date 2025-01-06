
#include <cstdio>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include <filesystem>

#include "JsonFunctions.h"

void separateFields(
    const std::string &line,
    const char* delimiter,
    std::vector< std::string > &lineEntries)
{
  lineEntries.clear();

  int idx0=0;
  int idx1=0;
  std::string entry;
  while(idx1 != std::string::npos){
    idx1 = line.find_first_of(delimiter,idx0);

    if(idx1 != std::string::npos){
      std::string entry = line.substr(idx0,idx1-idx0);
      lineEntries.push_back(entry);
      idx0=idx1+1;
    }else{
      std::string entry = line.substr(idx0,line.length()-idx0);
      lineEntries.push_back(entry);
    }
  }
};


int main (int argc, char* argv[]) {

  std::string inputCsvFileName;
  std::string outputJsonFileName; 
  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will read in a csv file and convert it "
    "to a json file. The first line must be a header followed by a line"
    "that defines the type: s - string, f-float",' ', "0.0");


    TCLAP::ValueArg<std::string> inputCsvFileNameInput("i",
      "input_csv_file", 
      "The path and file name of the input csv file to process",
      true,"","string");

    cmd.add(inputCsvFileNameInput);

    TCLAP::ValueArg<std::string> outputJsonFileNameInput("o",
      "output_json_file", 
      "The path and file name of the output json file that will be written",
      true,"","string");

    cmd.add(outputJsonFileNameInput);

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    inputCsvFileName          = inputCsvFileNameInput.getValue();
    outputJsonFileName        = outputJsonFileNameInput.getValue();
    verbose                   = verboseInput.getValue();


    if(verbose){
      std::cout << "  Input CSV File" << std::endl;
      std::cout << "    " << inputCsvFileName << std::endl;
      
      std::cout << "  Output Json File" << std::endl;
      std::cout << "    " << outputJsonFileName << std::endl;
  
    }

  } catch (TCLAP::ArgException &e)  // catch exceptions
	{ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }


  std::ifstream csvFile(inputCsvFileName);
  bool success = true;

  if(csvFile.is_open()){

    std::string line;

    std::getline(csvFile,line);
    std::vector< std::string > columnHeaders;
    separateFields(line, ",", columnHeaders);

    std::getline(csvFile,line);
    std::vector< std::string > dataTypes;
    separateFields(line, ",", dataTypes);


    std::vector< std::string > lineEntries;

    nlohmann::ordered_json jsonData;

    while(std::getline(csvFile,line)){


      nlohmann::ordered_json jsonEntry=nlohmann::ordered_json::object();

      separateFields(line,",",lineEntries);

      for(size_t i=0; i<lineEntries.size();++i){
        if(dataTypes[i].compare("s")==0){
          jsonEntry.push_back({columnHeaders[i],lineEntries[i]});
        }else if(dataTypes[i].compare("f")==0){
          double entry = std::nan("1");
          if(lineEntries[i].length() > 0){
            entry=std::stod(lineEntries[i]);
          }
          jsonEntry.push_back({columnHeaders[i],entry});
        }else{
          std::cout << "Error: type designations are limited to s or f, but "
                    << "instead this was entered: "
                    << dataTypes[i] 
                    << std::endl;
        }
      }
      jsonData.push_back(jsonEntry);
    }


    std::ofstream outputFileStream(outputJsonFileName,
        std::ios_base::trunc | std::ios_base::out);
    outputFileStream << jsonData;
    outputFileStream.close();
  }else{
    success=false;
    std::cout << "Error: could not open "
              << inputCsvFileName
              << std::endl;              
  }

  if(success){
    std::cout << "success" << std::endl;
  }else{
    std::cout << "failed to convert csv-to-json" << std::endl;
  }
  return 0;
}
