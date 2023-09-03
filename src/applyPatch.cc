
#include <cstdio>
#include <fstream>
#include <string>
#include <regex>

#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include <filesystem>
#include "StringToolkit.h"
#include "CurlToolkit.h"

unsigned int COLUMN_WIDTH=30;

int main (int argc, char* argv[]) {

  std::string apiKey;
  std::string eodUrlTemplate;

  std::string exchangeCode;
  std::string patchFileName;
  std::string fundamentalFolder;

  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will read in a patch file and will "
    "update the fundamental data files and fields indicated in the patch "
    "file. When the update involves the PrimaryTicker, the function will "
    "download this file from EOD so that upon completion the fundamental "
    "data folder is ready for further processing."
    ,' ', "0.0");

    TCLAP::ValueArg<std::string> apiKeyInput("k","EOD_API_KEY", 
      "The EOD API KEY of your account at https://eodhistoricaldata.com/",
      true,"","string");

    cmd.add(apiKeyInput);

    TCLAP::ValueArg<std::string> eodUrlInput("u","EOD_API_URL", 
      "The specific EOD API URL. For example: https://eodhistoricaldata.com/api"
      "/exchanges-list/?api_token={YOUR_API_KEY}",
      true,"","string");

    cmd.add(eodUrlInput);


    TCLAP::ValueArg<std::string> fundamentalFolderInput("f",
      "fundamental_data_folder_path", 
      "The path to the folder that contains the fundamental data json files from "
      "https://eodhistoricaldata.com/ to analyze",
      true,"","string");

    cmd.add(fundamentalFolderInput);


    TCLAP::ValueArg<std::string> patchFileNameInput("p",
      "patch_file_name", 
      "The full path and file name for the file produced by the generatePatch "
      "function (e.g. STU.patch.json).",
      true,"","string");

    cmd.add(patchFileNameInput);

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","EXCHANGE_CODE", 
      "The exchange code. For example: US",
      true,"","string");

    cmd.add(exchangeCodeInput);  


    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);

    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    apiKey                    = apiKeyInput.getValue();
    eodUrlTemplate            = eodUrlInput.getValue();
    patchFileName             = patchFileNameInput.getValue();
    fundamentalFolder         = fundamentalFolderInput.getValue();
    exchangeCode              = exchangeCodeInput.getValue();
    verbose                   = verboseInput.getValue();

    if(verbose){

      std::cout << "  Patch File Name" << std::endl;
      std::cout << "    " << patchFileName << std::endl;

      std::cout << "  Fundamental Data Folder" << std::endl;
      std::cout << "    " << fundamentalFolder << std::endl;
            
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

      std::cout << "  API Key" << std::endl;
      std::cout << "    " << apiKey << std::endl;

      std::cout << "  EOD Url Template" << std::endl;
      std::cout << "    " << eodUrlTemplate << std::endl;


    }
  } catch (TCLAP::ArgException &e)  // catch exceptions
	{ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }


  std::ifstream patchFileStream(patchFileName.c_str());

  using json = nlohmann::ordered_json;
  json patchListData = json::parse(patchFileStream);

  unsigned int patchCount = 0;
  for(auto& patchData : patchListData){
    ++patchCount;
    if(verbose){
      std::cout << std::endl;
      std::cout << patchCount << ". " 
        << '\t' << patchData["Code"].get<std::string>() 
        << "."  << patchData["Exchange"].get<std::string>() 
        << std::endl
        << '\t' << patchData["Name"].get<std::string>() << std::endl
        << '\t' << patchData["PatchName"].get<std::string>() 
        << '\t' <<"(PrimaryTicker)" << std::endl
        << '\t' << patchData["PrimaryTicker"].get<std::string>() 
        << std::endl;
    }

    //Update the primary ticker field
    std::string updFileName = fundamentalFolder;
    updFileName.append(patchData["Code"].get<std::string>());
    updFileName.append(".");
    updFileName.append(patchData["Exchange"].get<std::string>());
    updFileName.append(".json");

    std::ifstream updFileInputStream(updFileName.c_str());
    json updFileData = json::parse(updFileInputStream);    
    updFileInputStream.close();

    updFileData["General"]["PrimaryTicker"]=patchData["PrimaryTicker"].get<std::string>();

    std::ofstream updFileOutputStream(updFileName.c_str(), 
                std::ios_base::trunc | std::ios_base::out);
    updFileOutputStream << updFileData;
    updFileOutputStream.close();

    if(verbose){
      std::cout  << '\t' 
                 << patchData["Code"].get<std::string>()
                 << "."  
                 << patchData["Exchange"].get<std::string>() 
                 << " updated"
                 << std::endl;
    }

    //Download the primary ticker
    std::string eodUrl = eodUrlTemplate;
    std::string eodTicker = patchData["PrimaryTicker"].get<std::string>();
    unsigned int indexPoint = eodTicker.find('.');
    std::string eodExchange = eodTicker.substr(indexPoint+1,eodTicker.length());
    std::string ticker = eodTicker.substr(0,indexPoint);

    StringToolkit::findAndReplaceString(eodUrl,"{YOUR_API_TOKEN}",apiKey);  
    StringToolkit::findAndReplaceString(eodUrl,"{EXCHANGE_CODE}",eodExchange);
    StringToolkit::findAndReplaceString(eodUrl,"{TICKER_CODE}",ticker);

    std::string primaryFileName = eodTicker;
    primaryFileName.append(".json");

    bool successTickerDownload = 
      CurlToolkit::downloadJsonFile(eodUrl,fundamentalFolder,primaryFileName,false);

    if(verbose){
      if(successTickerDownload){
        std::cout   << '\t' 
                    << eodTicker << " downloaded" << std::endl;
      }else{
        std::cout   << '\t' 
                    << eodTicker  << " failed to download" << std::endl;      }      
    }
  }

  std::cout << "success" << std::endl;

  return 0;
}
