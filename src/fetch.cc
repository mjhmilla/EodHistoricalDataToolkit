#include <sys/stat.h>
#include <cstdio>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>


#include "FinancialAnalysisToolkit.h"
#include "JsonFunctions.h"
#include "CurlToolkit.h"

unsigned int MODE_FETCH_SINGLE_FILE               = 0;
unsigned int MODE_FETCH_MULTIPLE_TICKER_FILES     = 1;
unsigned int MODE_FETCH_MULTIPLE_EXCHANGE_FILES   = 2;




int main (int argc, char* argv[]) {

  std::string apiKey;
  std::string eodUrlTemplate;
  std::string exchangeCode;
  std::string exchangeListFileName;
  std::string tickerFileListPath;
  int firstListEntry;
  int lastListEntry;
  std::string outputFolder;
  std::string outputFileName;
  std::string outputPrimaryTickerFileName;
  bool gapFillPartialDownload;
  bool skipExistingFiles;  
  bool verbose;

  unsigned int mode;

  try{
    TCLAP::CmdLine cmd("The command fetch will download json data from "
    "the web API of https://eodhistoricaldata.com/ and save it to a folder. "
    "The command can be used to fetch individual files (e.g. exchange list, "
    "exchange-symbol-list) or multiple files (e.g. the data from each ticker "
    "in an exchange, or the symbol-list for each exchange listed in an "
    "list of exchanges.).)"
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

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","exchange_code", 
      "The exchange code. For example: US",
      false,"","string");

    cmd.add(exchangeCodeInput);    


    TCLAP::ValueArg<std::string> exchangeListFileNameInput("l","exchange_list", 
      "The exchanges-list json file generated by EOD",
      false,"","string");

    cmd.add(exchangeListFileNameInput);    

    TCLAP::ValueArg<std::string> tickerFileListPathInput("t","ticker_list_file", 
      "Download the data for the list of tickers contained in this json file",
      false,"","string");

    cmd.add(tickerFileListPathInput);

    TCLAP::ValueArg<int> firstListEntryInput("s","ticker_list_start", 
      "The index of the first entry on the list to download",
      false,-1,"int");

    cmd.add(firstListEntryInput);

    TCLAP::ValueArg<int> lastListEntryInput("e","ticker_list_end", 
      "The index of the last entry on the list to download",
      false,-1,"int");

    cmd.add(lastListEntryInput);    

    TCLAP::ValueArg<std::string> outputFolderInput("f","folder", 
      "path to the folder that will store the downloaded json files",
      true,"","string");

    cmd.add(outputFolderInput);


    TCLAP::ValueArg<std::string> outputFileNameInput("n","file_name", 
      "name of the output file",false,"","string");

    cmd.add(outputFileNameInput);

    TCLAP::SwitchArg gapFillPartialDownloadInput("g","gapfill",
      "Download the missing files to fill the gaps from an incomplete download",
       false);
    cmd.add(gapFillPartialDownloadInput); 

    TCLAP::SwitchArg skipExistingFilesInput("i","skip_existing_files",
      "Existing primary ticker files will not be downloaded again", false);

    cmd.add(skipExistingFilesInput); 

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    apiKey                    = apiKeyInput.getValue();
    eodUrlTemplate            = eodUrlInput.getValue();
    exchangeCode              = exchangeCodeInput.getValue();
    exchangeListFileName  = exchangeListFileNameInput.getValue();
    tickerFileListPath        = tickerFileListPathInput.getValue();
    firstListEntry            = firstListEntryInput.getValue();
    lastListEntry             = lastListEntryInput.getValue();
    outputFolder              = outputFolderInput.getValue();
    outputFileName            = outputFileNameInput.getValue();
    gapFillPartialDownload    = gapFillPartialDownloadInput.getValue();
    skipExistingFiles         = skipExistingFilesInput.getValue();
    verbose                   = verboseInput.getValue();

    if(tickerFileListPath.length()==0 
        && outputFileName.length()==0 
        && exchangeListFileName.length()==0){
      throw std::invalid_argument(
        " One of the following tags must be provided to define "
        "an output file path: output file name (-n), "
        "ticker file list path (-t), or an exchange list file (-l)");         
    }

    if( (    tickerFileListPath.length() !=0 
          || exchangeListFileName.length() !=0 ) && 
        (firstListEntry != -1 && lastListEntry != -1) ){
          if(lastListEntry < firstListEntry){
            throw std::invalid_argument(
              "The last list entry (-e) is smaller than the"
              " first list entry (-s).");         
          }
    }    

    if(exchangeCode.length() > 0 && exchangeListFileName.length() > 0){
      throw std::invalid_argument(
        "Cannot set both the exchange code (-x) and exchange list "
        "file name (-l).");               
    }

    if(tickerFileListPath.length() >0 && exchangeListFileName.length() >0){
      throw std::invalid_argument(
        "Cannot set both the ticker list file (-t) and the exchange "
        "list file (-l).");               
    }



    mode=MODE_FETCH_SINGLE_FILE;

    if(tickerFileListPath.length() > 0){
      mode = MODE_FETCH_MULTIPLE_TICKER_FILES;
    }
    if(exchangeListFileName.length() > 0){
      mode = MODE_FETCH_MULTIPLE_EXCHANGE_FILES;
    }

  

    if(verbose){
      std::cout << "************" <<std::endl;
      std::cout << "Todo: Remove gapFillPartialDownload" << std::endl;
      std::cout << "************" <<std::endl;

      std::cout << "  API Key" << std::endl;
      std::cout << "    " << apiKey << std::endl;

      std::cout << "  EOD Url Template" << std::endl;
      std::cout << "    " << eodUrlTemplate << std::endl;

      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

      std::cout << "  Exchange List File" << std::endl;
      std::cout << "    " << exchangeListFileName << std::endl;

      if(tickerFileListPath.length()>0){
        std::cout << "  Ticker list file" << std::endl;
        std::cout << "    " << tickerFileListPath << std::endl;

        if(firstListEntry != -1){
          std::cout << "  First list entry" << std::endl;
          std::cout << "    " << firstListEntry << std::endl;
        }
        if(lastListEntry != -1){
          std::cout << "  Last list entry" << std::endl;
          std::cout << "    " << lastListEntry << std::endl;
        }
      }

      std::cout << "  Fill the gaps in an incomplete download" << std::endl;
      std::cout << "    " << gapFillPartialDownload << std::endl;

      std::cout << "  Output Folder" << std::endl;
      std::cout << "    " << outputFolder << std::endl;

      if(tickerFileListPath.length()==0){
        std::cout << "  Output File Name" << std::endl;
        std::cout << "    " << outputFileName << std::endl;
      }

      std::cout << "  Skip existing files" << std::endl;
      std::cout << "    " << skipExistingFiles << std::endl;
    }
  } catch (TCLAP::ArgException &e){ 
    std::cerr << "TCLAP::ArgException: "   << e.error() 
              << " for arg "  << e.argId() << std::endl;
    abort();
  } catch (std::invalid_argument &e){
    std::cerr << "std::invalid_argument: " << e.what() << std::endl; 
    abort();
  }

  if(mode == MODE_FETCH_SINGLE_FILE){

    std::string eodUrl = eodUrlTemplate;
    StringToolkit::findAndReplaceString(eodUrl,"{YOUR_API_TOKEN}",apiKey);  
    StringToolkit::findAndReplaceString(eodUrl,"{EXCHANGE_CODE}",exchangeCode);

    bool success = 
      CurlToolkit::downloadJsonFile(eodUrl,outputFolder, outputFileName, 
        skipExistingFiles,verbose);

    if(verbose && success == true){
      std::cout << '\t' << outputFileName << std::endl;
    }    
    if( success == false){
      std::cerr << "Error: CurlToolkit::downloadJsonFile failed to get" 
                << std::endl;
      std::cerr << '\t' << eodUrl << std::endl;
      std::cerr << '\t' << outputFileName << std::endl;
    }

  }

  if(mode==MODE_FETCH_MULTIPLE_TICKER_FILES){

    std::ifstream tickerFileListPathStream(tickerFileListPath.c_str());

    using json = nlohmann::ordered_json;
    json tickerListData = json::parse(tickerFileListPathStream);
    int count = 0;
    if(verbose){
      std::cout << std::endl;
      std::cout << "Fetching data on the ticker list ..." << std::endl;
    }
    
    for(auto& it : tickerListData){
      bool processEntry=true;
      if(count < firstListEntry && firstListEntry != -1){
        processEntry=false;
      }      
      if(count > lastListEntry && lastListEntry != -1){
        processEntry=false;
      }



      if(processEntry){
        std::string eodUrl = eodUrlTemplate;
        std::string ticker = it["Code"];
        StringToolkit::findAndReplaceString(eodUrl,"{YOUR_API_TOKEN}",apiKey);  
        StringToolkit::findAndReplaceString(eodUrl,"{EXCHANGE_CODE}",exchangeCode);
        StringToolkit::findAndReplaceString(eodUrl,"{TICKER_CODE}",ticker);

        std::string fileName = ticker;
        fileName.append(".");
        fileName.append(exchangeCode);
        fileName.append(".json");

        bool fileExists=false;

        if(gapFillPartialDownload == true){
          //Check if the file has been downloaded already.
          std::string existingFilePath = outputFolder;
          existingFilePath.append(fileName);
          fileExists = std::filesystem::exists(existingFilePath.c_str());
        }

        bool successTickerDownload=false;
        if( (fileExists == false && gapFillPartialDownload == true) 
                                 || gapFillPartialDownload == false){ 
                                  
          successTickerDownload = 
            CurlToolkit::downloadJsonFile(eodUrl, outputFolder,fileName, 
                                                skipExistingFiles,false);

          if(successTickerDownload == false){
            std::cout << count << "." 
                      << '\t' << fileName << std::endl 
                      << '\t' << "Error: failed to download" << std::endl
                      << '\t' << eodUrl << std::endl;
          } 
          if(verbose && successTickerDownload == true){
            std::cout << count << "." << '\t' << fileName << std::endl;
          }         
        
        }

        if( (fileExists == true && gapFillPartialDownload == true) 
           || successTickerDownload==true){
          std::string primaryEodTickerName("");
          JsonFunctions::getPrimaryTickerName(outputFolder, 
                                        fileName, primaryEodTickerName);

          //If this is not the primary ticker, then we need to download the 
          //primary ticker file  
          std::string eodTicker=ticker;
          eodTicker.append(".").append(exchangeCode);

          if(std::strcmp(eodTicker.c_str(),primaryEodTickerName.c_str()) != 0 
            && primaryEodTickerName.length() > 0){
            std::string eodUrlPrimary = eodUrlTemplate;        
            unsigned int idx = primaryEodTickerName.find(".");
            std::string tickerPrimary =primaryEodTickerName.substr(0,idx);          
            std::string exchangeCodePrimary =
              primaryEodTickerName.substr(idx+1,primaryEodTickerName.length()-1);

            StringToolkit::findAndReplaceString(eodUrlPrimary,"{YOUR_API_TOKEN}",apiKey);  
            StringToolkit::findAndReplaceString(eodUrlPrimary,"{EXCHANGE_CODE}",exchangeCodePrimary);
            StringToolkit::findAndReplaceString(eodUrlPrimary,"{TICKER_CODE}",tickerPrimary);

            std::string fileNamePrimary = primaryEodTickerName; //This will include the exchange code
            fileNamePrimary.append(".json");

            bool filePrimaryExists=false;
            if(gapFillPartialDownload == true){
              //Check if the file has been downloaded already.
              std::string existingPrimaryFilePath = outputFolder;
              existingPrimaryFilePath.append(fileNamePrimary);
              filePrimaryExists 
                = std::filesystem::exists(existingPrimaryFilePath.c_str());
            } 

            if((filePrimaryExists==false && gapFillPartialDownload==true) 
                || gapFillPartialDownload == false){           
              bool successPrimaryDownload = 
                CurlToolkit::downloadJsonFile(eodUrlPrimary,outputFolder,
                                  fileNamePrimary,skipExistingFiles,false);  
                 

              if( successPrimaryDownload == false ){
                std::cerr << "Error: CurlToolkit::downloadJsonFile: " 
                          << std::endl;
                std::cerr << '\t' << fileNamePrimary << std::endl;
                std::cerr << '\t' << eodUrlPrimary << std::endl;
              }                          
              if(verbose && successPrimaryDownload == true){
                std::cout << count << ". (PrimaryTicker)" << '\t' 
                          << fileNamePrimary << std::endl;
              }  
            }
          }
        }

               
      }
      ++count;
    }
    
  }

  if(mode==MODE_FETCH_MULTIPLE_EXCHANGE_FILES){
    std::ifstream exchangeListPathStream(exchangeListFileName.c_str());

    using json = nlohmann::ordered_json;
    json exchangeListData = json::parse(exchangeListPathStream);
    int count = 0;
    if(verbose){
      std::cout << std::endl;
      std::cout << "Fetching data on the exchange list ..." << std::endl;
    }

    for(auto& it : exchangeListData){
      bool processEntry=true;
      if(count < firstListEntry && firstListEntry != -1){
        processEntry=false;
      }      
      if(count > lastListEntry && lastListEntry != -1){
        processEntry=false;
      }

      if(processEntry){
        std::string eodUrl = eodUrlTemplate;
        std::string exchangeCode = it["Code"];
        StringToolkit::findAndReplaceString(eodUrl,"{YOUR_API_TOKEN}",apiKey);  
        StringToolkit::findAndReplaceString(eodUrl,"{EXCHANGE_CODE}",exchangeCode);

        std::string fileName = exchangeCode;
        fileName.append(".json");

        bool fileExists=false;

        if(gapFillPartialDownload == true){
          //Check if the file has been downloaded already.
          std::string existingFilePath = outputFolder;
          existingFilePath.append(fileName);
          fileExists = std::filesystem::exists(existingFilePath.c_str());
        }

        bool successTickerDownload=false;
        if( (fileExists == false && gapFillPartialDownload == true) 
                                 || gapFillPartialDownload == false){ 
                                  
          successTickerDownload = 
            CurlToolkit::downloadJsonFile(eodUrl,outputFolder,fileName,
              skipExistingFiles,false);

          if(successTickerDownload == false){
            std::cout << count << "." 
                      << '\t' << fileName << std::endl 
                      << '\t' << "Error: failed to download" << std::endl
                      << '\t' << eodUrl << std::endl;
          } 
          if(verbose && successTickerDownload == true){
            std::cout << count << "." << '\t' << fileName << std::endl;
          }         
        
        }
      }      
      ++count;
    }
  }

  if(verbose){
    std::cout << "success" << std::endl;
  }

  
  return 0;
}
