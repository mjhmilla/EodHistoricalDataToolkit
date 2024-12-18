
#include <cstdio>
#include <fstream>
#include <string>
#include <cmath>
#include <filesystem>

#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include "FinancialAnalysisToolkit.h"
#include "JsonFunctions.h"


bool applyFilter(std::string &tickerFileName,  
                 std::string &fundamentalFolder,
                 std::string &historicalFolder,
                 std::string &calculateDataFolder,
                 nlohmann::ordered_json &marketReportConfig,
                 bool replaceNansWithMissingData,
                 bool verbose){

  bool valueFilter = true;



  bool filterFieldExists = marketReportConfig.contains("filter");

  if(filterFieldExists){
    for( auto const &el : marketReportConfig["filter"].items() ){
      //
      // Retreive the filter data
      //
      std::string filterName(el.key());

      std::string folder; 
      JsonFunctions::getJsonString(el.value()["folder"],folder);

      std::vector< std::string > fieldVector;
      std::string fieldName("");
      for( auto const &fieldEntry : el.value()["field"]){
        JsonFunctions::getJsonString(fieldEntry,fieldName);
        fieldVector.push_back(fieldName);
      }

      std::string conditionName("");
      JsonFunctions::getJsonString(el.value()["condition"],conditionName);

      std::string operatorName("");
      JsonFunctions::getJsonString(el.value()["operator"],operatorName);

      std::string valueType("");
      JsonFunctions::getJsonString(el.value()["valueType"],valueType);


      std::vector< std::string > valueStringVector;
      std::string valueStringName;
      if(valueType == "string"){
        for( auto const &fieldEntry : el.value()["values"]){
          JsonFunctions::getJsonString(fieldEntry,valueStringName);
          valueStringVector.push_back(valueStringName);
        }  
      }

      std::vector< double > valueFloatVector;
      double valueFloatName;
      if(valueType == "float"){
        for( auto const &fieldEntry : el.value()["values"]){
          valueFloatName = JsonFunctions::getJsonFloat(fieldEntry);
          valueFloatVector.push_back(valueFloatName);
        }  
      }

      //
      // Fetch the target data and apply the filter
      //
      std::string fullPathFileName("");      
      if(folder == "fundamentalData"){
        fullPathFileName = fundamentalFolder;                
      }else if(folder == "historicalData"){
        fullPathFileName = historicalFolder;                        
      }else if(folder == "calculateData"){
        fullPathFileName = calculateDataFolder;                      
      }else{
        std::cerr << "Error: in filter " << filterName 
                  << " the folder field is " << folder 
                  << " but should be fundamentalData, historicalData,"
                  << " or calculateData" 
                  << std::endl;
        std::abort();                  
      }

      fullPathFileName.append(tickerFileName);

      nlohmann::ordered_json targetJsonTable;
      bool loadedConfiguration = 
        JsonFunctions::loadJsonFile(fullPathFileName,
                                    targetJsonTable,
                                    verbose); 

      std::string valueString;
      double valueFloat;
      std::vector < bool > valueBoolVector;

      if(valueType == "string"){
        JsonFunctions::getJsonString(targetJsonTable, fieldVector, valueString);

        //Evaluate all of the individual values against the target
        for(size_t i = 0; i <  valueStringVector.size(); ++i){
          if(conditionName == "=="){
            bool equalityTest = (valueString == valueStringVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == "!="){
            bool equalityTest = (valueString != valueStringVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else{
            std::cerr << "Error: in filter " << filterName 
                  << " the condition field should be " 
                  << " == or != for a string"
                  << " but is instead " << conditionName  
                  << std::endl;
            std::abort();   
          }        
        }

      }else if(valueType == "float"){
        valueFloat = JsonFunctions::getJsonFloat(targetJsonTable, fieldVector,
                                                 replaceNansWithMissingData);

        //Evaluate all of the individual values against the target
        for(size_t i = 0; i <  valueFloatVector.size(); ++i){
          if(conditionName == "=="){
            bool equalityTest = (valueFloat == valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == "!="){
            bool equalityTest = (valueFloat != valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == ">"){
            bool equalityTest = (valueFloat > valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == "<"){
            bool equalityTest = (valueFloat < valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == ">="){
            bool equalityTest = (valueFloat >= valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else if (conditionName == "<="){
            bool equalityTest = (valueFloat <= valueFloatVector[i]);
            valueBoolVector.push_back(equalityTest);
          }else{
            std::cerr << "Error: in filter " << filterName 
                  << " the condition field should be one of" 
                  << " (==, !=, >, <, >=, <=) for a float"
                  << " but is instead " << conditionName  
                  << std::endl;
            std::abort();   
          }        
        }


      }else{
        std::cerr << "Error: in filter " << filterName 
                  << " valueType should be string or float but is instead" 
                  << valueType 
                  << std::endl;        
      }


      //Apply the operator between all of the values to yield the final 
      //filter value
      valueFilter = valueBoolVector[0];
      for(size_t i = 1; i < valueBoolVector.size(); ++i){
        if(operatorName == "||"){
          valueFilter = (valueFilter || valueBoolVector[i]);
        }else if( operatorName == "&&"){
          valueFilter = (valueFilter && valueBoolVector[i]);
        }else{
          std::cerr << "Error: in filter " << filterName 
                << " the operator field should be " 
                << " || or && for a string"
                << " but is instead " << operatorName  
                << std::endl;
          std::abort();             
        }
      }



    }


  }

  return valueFilter;
};


//==============================================================================
int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string calculateDataFolder;
  std::string fundamentalFolder;
  std::string historicalFolder;
  std::string marketReportConfigurationFilePath;  
  std::string tickerReportFolder;
  std::string marketReportFolder;  
  std::string dateOfTable;

  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will produce a market report in the form of "
    "text and tables about the companies that are best ranked."
    ,' ', "0.0");

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","exchange_code", 
      "The exchange code. For example: US",
      false,"","string");
    cmd.add(exchangeCodeInput);  

    TCLAP::ValueArg<std::string> tickerReportFolderOutput("t","ticker_report_folder_path", 
      "The path to the folder will contains the ticker reports.",
      true,"","string");
    cmd.add(tickerReportFolderOutput);

    TCLAP::ValueArg<std::string> marketReportFolderOutput("o","market_report_folder_path", 
      "The path to the folder will contains the market report.",
      true,"","string");
    cmd.add(marketReportFolderOutput);    

    TCLAP::ValueArg<std::string> calculateDataFolderInput("a","calculate_folder_path", 
      "The path to the folder contains the output "
      "produced by the calculate method",
      true,"","string");
    cmd.add(calculateDataFolderInput);

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

    TCLAP::ValueArg<std::string> dateOfTableInput("d","date", 
      "Date used to produce the dicounted cash flow model detailed output.",
      false,"","string");
    cmd.add(dateOfTableInput);

    TCLAP::ValueArg<std::string> marketReportConfigurationFilePathInput("c",
      "market_report_configuration_file", 
      "The path to the json file that contains the names of the metrics "
      " to plot.",
      true,"","string");

    cmd.add(marketReportConfigurationFilePathInput);    

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    exchangeCode              = exchangeCodeInput.getValue();  
    marketReportConfigurationFilePath 
                              = marketReportConfigurationFilePathInput.getValue();      
    calculateDataFolder       = calculateDataFolderInput.getValue();
    fundamentalFolder         = fundamentalFolderInput.getValue();
    historicalFolder          = historicalFolderInput.getValue();    
    tickerReportFolder        = tickerReportFolderOutput.getValue();
    marketReportFolder        = marketReportFolderOutput.getValue();    
    dateOfTable               = dateOfTableInput.getValue();
    verbose                   = verboseInput.getValue();

    if(verbose){
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;


      if(dateOfTable.length()>0){
        std::cout << "  Date used to calculate tabular output" << std::endl;
        std::cout << "    " << dateOfTable << std::endl;
      }

      std::cout << "  Calculate Data Input Folder" << std::endl;
      std::cout << "    " << calculateDataFolder << std::endl;

      std::cout << "  Fundamental Data Folder" << std::endl;
      std::cout << "    " << fundamentalFolder << std::endl;

      std::cout << "  Historical Data Folder" << std::endl;
      std::cout << "    " << historicalFolder << std::endl;

      std::cout << "  Ticker Report Folder" << std::endl;
      std::cout << "    " << tickerReportFolder << std::endl;  

      std::cout << "  Market Report Configuration File" << std::endl;
      std::cout << "    " << marketReportConfigurationFilePath << std::endl;          

      std::cout << "  Market Report Folder" << std::endl;
      std::cout << "    " << marketReportFolder << std::endl;  

    }

  } catch (TCLAP::ArgException &e){ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }

  bool replaceNansWithMissingData = true;

  //Load the report configuration file
  nlohmann::ordered_json marketReportConfig;
  bool loadedConfiguration = 
    JsonFunctions::loadJsonFile(marketReportConfigurationFilePath,
                                marketReportConfig,
                                verbose);
                                  

  if(!loadedConfiguration){
    std::cerr << "Error: cannot open " << marketReportConfigurationFilePath 
              << std::endl;
    
  }



  if(loadedConfiguration){
    std::vector< std::string > processedTickers;

    int totalFileCount = 0;
    int validFileCount = 0;
    std::string analysisExt = ".json";  



    //Go through every ticker file in the calculate folder:
    //1. Create a ticker folder in the reporting folder
    //2. Save to this ticker folder:
    //  a. The pdf of the ticker's plots
    //  b. The LaTeX file for the report (as an input)
    //  c. A wrapper LaTeX file to generate an individual report

    std::vector< std::string > filteredTickers;

    for (const auto & file 
          : std::filesystem::directory_iterator(calculateDataFolder)){  

      ++totalFileCount;
      bool validInput = true;

      std::string fileName   = file.path().filename();

      std::size_t fileExtPos = fileName.find(analysisExt);

      if(verbose){
        std::cout << totalFileCount << '\t' << fileName << std::endl;
      }
      
      if( fileExtPos == std::string::npos ){
        validInput=false;
        if(verbose){
          std::cout << '\t' 
                    << "Skipping " << fileName 
                    << " should end in .json but this does not:" << std::endl; 
        }
      }

      //Check to see if this ticker passes the filter
      bool tickerPassesFilter=false;
      if(validInput){
        tickerPassesFilter = applyFilter( fileName,  
                                  fundamentalFolder,
                                  historicalFolder,
                                  calculateDataFolder,
                                  marketReportConfig,
                                  replaceNansWithMissingData,
                                  verbose);
      }    

      if(tickerPassesFilter){
        filteredTickers.push_back(fileName);
      }
      if(verbose){
        if(tickerPassesFilter){
          std::cout << '\t'
                    << "passed"
                    << std::endl;
        }else{
          std::cout << '\t'
                    << "filtered out"
                    << std::endl;
        }
      }

    }

    if(verbose){
      std::cout << '\n' << '\n'
                << filteredTickers.size() << '\t' 
                << "files passed the filter"
                << '\n' 
                << totalFileCount - filteredTickers.size() << '\t' 
                << "files removed"
                << std::endl;

      std::cout << '\n' << '\n'
                << "Tickers that passed the filter: " << std::endl;
      for(size_t i=0; i< filteredTickers.size(); ++i){
        std::cout << i << '\t' << filteredTickers[i] << std::endl;
      }                

    }
  }

  return 0;
}
