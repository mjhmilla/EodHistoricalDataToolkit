
#include <cstdio>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>
#include <curl/curl.h>


unsigned int COLUMN_WIDTH = 30;

namespace
{
  std::size_t callback(
    const char* in,
    std::size_t size,
    std::size_t num,
    std::string* out)
  {
    const std::size_t totalBytes(size * num);
    out->append(in, totalBytes);
    return totalBytes;
  }
}

void removeFromString(std::string& str,
               const std::string& removeStr)
{
  std::string::size_type pos = 0u;
  pos = str.find(removeStr, pos);
  while(pos != std::string::npos){
     str.erase(pos, removeStr.length());
     pos += removeStr.length();
     pos = str.find(removeStr, pos);
  }
}

void findAndReplaceString(std::string& str,
               const std::string& findStr,
               const std::string& replaceStr)
{
  std::string::size_type pos = 0u;
  pos = str.find(findStr, pos);
  while(pos != std::string::npos){
     str.replace(pos, findStr.length(),replaceStr);
     pos += replaceStr.length();
     pos = str.find(findStr, pos);
  }
}

bool downloadJsonFile(std::string &eodUrl, 
                std::string &outputFolder, 
                std::string &outputFileName, 
                bool verbose){

    bool success = true;

    if(verbose){
      std::cout << std::endl;
      std::cout << "    Contacting" << std::endl;
      std::cout << "    " << eodUrl << std::endl;
    }

    CURL* curl = curl_easy_init();

    // Set remote URL.
    curl_easy_setopt(curl, CURLOPT_URL, eodUrl.c_str());

    // Don't bother trying IPv6, which would increase DNS resolution time.
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    // Don't wait forever, time out after 10 seconds.
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

    // Follow HTTP redirects if necessary.
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Response information.
    long httpCode(0);
    std::unique_ptr<std::string> httpData(new std::string());

    // Hook up data handling function.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);

    // Hook up data container (will be passed as the last parameter to the
    // callback handling function).  Can be any pointer type, since it will
    // internally be passed as a void pointer.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());

    // Run our HTTP GET command, capture the HTTP response code, and clean up.
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if(verbose){
      std::cout << "    http response code" << std::endl;
      std::cout << "    " << httpCode << std::endl;
    }

    if (httpCode == 200)
    {
      //read out all of the json keys
      using json = nlohmann::ordered_json;
      //std::ifstream f("../json/AAPL.US.json");
      json jsonData = json::parse(*httpData.get());


      std::stringstream ss;
      ss << outputFolder << outputFileName;
      std::string outputFilePathName = ss.str();
      std::string removeStr("\"");
      removeFromString(outputFilePathName,removeStr);    

      //Write the file
      std::ofstream file(outputFilePathName);
      file << jsonData;
      file.close();
      if(verbose){    
        std::cout << "    Wrote json to" << std::endl;
        std::cout << "    " << outputFileName << std::endl;
      }

    }else{
      success=false;
    }
    return success;

}

int main (int argc, char* argv[]) {

  std::string apiKey;
  std::string eodUrlTemplate;
  std::string exchangeCode;
  std::string tickerFileListPath;
  std::string outputFolder;
  std::string outputFileName;
  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command fetch will download json data from "
    "the web API of https://eodhistoricaldata.com/ and save it to a folder. "
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

    TCLAP::ValueArg<std::string> exchangeCodeInput("e","EXCHANGE_CODE", 
      "The exchange code. For example: US",
      false,"","string");

    cmd.add(exchangeCodeInput);

    TCLAP::ValueArg<std::string> tickerFileListPathInput("t","ticker_list_file", 
      "Download the data for the list of tickers contained in this json file",
      false,"","string");

    cmd.add(tickerFileListPathInput);

    TCLAP::ValueArg<std::string> outputFolderInput("f","folder", 
      "path to the folder that will store the downloaded json files",
      true,"","string");

    cmd.add(outputFolderInput);

    TCLAP::ValueArg<std::string> outputFileNameInput("n","file_name", 
      "name of the output file",false,"","string");

    cmd.add(outputFileNameInput);

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    apiKey              = apiKeyInput.getValue();
    eodUrlTemplate      = eodUrlInput.getValue();
    exchangeCode        = exchangeCodeInput.getValue();
    tickerFileListPath  = tickerFileListPathInput.getValue();
    outputFolder        = outputFolderInput.getValue();
    outputFileName      = outputFileNameInput.getValue();
    verbose             = verboseInput.getValue();

    if(tickerFileListPath.length()==0 && outputFileName.length()==0){
      throw std::invalid_argument(
        "An output file_name (-n) must be provided when"
        " a ticker_file_list (-t) is not provided");         
    }

    if(verbose){
      std::cout << "  API Key" << std::endl;
      std::cout << "    " << apiKey << std::endl;

      std::cout << "  EOD Url Template" << std::endl;
      std::cout << "    " << eodUrlTemplate << std::endl;

      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

      std::cout << "  Ticker list file" << std::endl;
      std::cout << "    " << tickerFileListPath << std::endl;

      std::cout << "  Output Folder" << std::endl;
      std::cout << "    " << outputFolder << std::endl;

      std::cout << "  Output File Name" << std::endl;
      std::cout << "    " << outputFileName << std::endl;

    }
  } catch (TCLAP::ArgException &e){ 
    std::cerr << "TCLAP::ArgException: "   << e.error() 
              << " for arg "  << e.argId() << std::endl;
    abort();
  } catch (std::invalid_argument &e){
    std::cerr << "std::invalid_argument: " << e.what() << std::endl; 
    abort();
  }




  if(tickerFileListPath.length() > 0){

    std::ifstream tickerFileListPathStream(tickerFileListPath.c_str());

    using json = nlohmann::ordered_json;
    json tickerListData = json::parse(tickerFileListPathStream);

    for(auto& it : tickerListData){
      std::string eodUrl = eodUrlTemplate;
      std::string ticker = it["Code"];
      findAndReplaceString(eodUrl,"{YOUR_API_TOKEN}",apiKey);  
      findAndReplaceString(eodUrl,"{EXCHANGE_CODE}",exchangeCode);
      findAndReplaceString(eodUrl,"{TICKER_CODE}",ticker);

      std::string fileName = ticker;
      fileName.append(".");
      fileName.append(exchangeCode);
      fileName.append(".json");
      bool success = downloadJsonFile(eodUrl,outputFolder,fileName,verbose);
      if( success == false){
        std::cerr << "Error: downloadJsonFile: " << eodUrl << std::endl;
      }

    }
    
  }else{

    std::string eodUrl = eodUrlTemplate;
    findAndReplaceString(eodUrl,"{YOUR_API_TOKEN}",apiKey);  
    findAndReplaceString(eodUrl,"{EXCHANGE_CODE}",exchangeCode);

    bool success = downloadJsonFile(eodUrl,outputFolder,outputFileName,verbose);
    if( success == false){
      std::cerr << "Error: downloadJsonFile" << std::endl;
    }

  }



  
  return 0;
}
