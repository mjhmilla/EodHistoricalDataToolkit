
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

int main (int argc, char* argv[]) {

  std::string apiKey;
  std::string eodUrl;
  std::string exchangeCode;
  std::string outputFolder;
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

    TCLAP::ValueArg<std::string> outputFolderInput("f","folder", 
      "path to the folder that will store the downloaded json files",true,"","string");

    cmd.add(outputFolderInput);

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    apiKey        = apiKeyInput.getValue();
    eodUrl        = eodUrlInput.getValue();
    exchangeCode  = exchangeCodeInput.getValue();
    outputFolder  = outputFolderInput.getValue();
    verbose       = verboseInput.getValue();

    if(verbose){
      std::cout << "  API Key" << std::endl;
      std::cout << "    " << apiKey << std::endl;

      std::cout << "  EOD Url" << std::endl;
      std::cout << "    " << eodUrl << std::endl;

      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

      std::cout << "  Output Folder" << std::endl;
      std::cout << "    " << outputFolder << std::endl;
    }
  } catch (TCLAP::ArgException &e)  // catch exceptions
	{ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }

  findAndReplaceString(eodUrl,"{YOUR_API_TOKEN}",apiKey);
  findAndReplaceString(eodUrl,"{EXCHANGE_CODE}",exchangeCode);

  if(verbose){
    std::cout << std::endl;
    std::cout << "    Contacting" << std::endl;
    std::cout << "    " << eodUrl << std::endl;
  }
  //use libcurl to get AAPL.US
  //const std::string url("https://eodhistoricaldata.com/api/fundamentals/AAPL.US?api_token=demo");

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

    //Make the file name {EXCHANGE}_{TICKER}.json
    std::stringstream ss;
    ss  << outputFolder << jsonData["General"]["PrimaryTicker"] << ".json";
    //Remove the quote ("") characters
    std::string fileName = ss.str();
    std::string removeStr("\"");
    removeFromString(fileName,removeStr);    

    
    //Write the file
    std::ofstream file(fileName);
    file << jsonData;
    if(verbose){    
      std::cout << "    Wrote json to" << std::endl;
      std::cout << "    " << fileName << std::endl;
    }

  }else{
    std::cout << "Error" << std::endl;
  }

  return 0;
}
