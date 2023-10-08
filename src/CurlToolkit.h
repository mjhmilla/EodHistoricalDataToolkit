#ifndef CURL_TOOLKIT
#define CURL_TOOLKIT

#include <filesystem>
#include <stdlib.h>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "StringToolkit.h"



unsigned int CURL_TIMEOUT_TIME_SECONDS = 20;
unsigned int DOWNLOAD_ATTEMPTS=2;


class CurlToolkit {

  public:

//  namespace
//  {
    static std::size_t callBack(const char* in,
                                std::size_t size,
                                std::size_t num,
                                std::string* out)
    {
      const std::size_t totalBytes(size * num);
      out->append(in, totalBytes);
      return totalBytes;
    };
//  }



    static bool downloadJsonFile( std::string &eodUrl, 
                                  std::string &outputFolder, 
                                  std::string &outputFileName,
                                  bool skipIfFileExists, 
                                  bool verbose){

      bool success = false;
      unsigned int downloadAttempts=0;

      std::stringstream ss;
      ss << outputFolder << outputFileName;
      std::string outputFilePathName = ss.str();
      std::string removeStr("\"");
      StringToolkit::removeFromString(outputFilePathName,removeStr); 

      bool fileExists=false;
      if(skipIfFileExists){
        std::filesystem::path filePath=outputFilePathName;
        fileExists = std::filesystem::exists(filePath); 
        if(fileExists){
          success=true;
        }       
      }      

      while(downloadAttempts < DOWNLOAD_ATTEMPTS && !success && !fileExists){

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
  
        // Don't wait forever, time out after 20 seconds.
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT_TIME_SECONDS);
  
        // Follow HTTP redirects if necessary.
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  
        // Response information.
        long httpCode(0);
        std::unique_ptr<std::string> httpData(new std::string());
  
        // Hook up data handling function.
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callBack);
  
        // Hook up data container (will be passed as the last parameter to the
        // callBack handling function).  Can be any pointer type, since it will
        // internally be passed as a void pointer.
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());
  
        // Run our HTTP GET command, capture the HTTP response code, and clean up.
        unsigned int retryAttemptCount = 0;
  
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_easy_cleanup(curl);
  
        if(verbose){
          std::cout << "    http response code" << std::endl;
          std::cout << "    " << httpCode << std::endl;
        }

        if (httpCode == 200)
        {
          success=true;
          //read out all of the json keys
          using json = nlohmann::ordered_json;
          //std::ifstream f("../json/AAPL.US.json");
          json jsonData = json::parse(*httpData.get());
  
  
    
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
  
        ++downloadAttempts;
      }
  
    return success;
  };

    static bool downloadHtmlToString( std::string &url, 
                                  std::string &updUrlContents, 
                                  bool verbose){

      bool success = false;
      unsigned int downloadAttempts=0;
      updUrlContents.clear();

      while(downloadAttempts < DOWNLOAD_ATTEMPTS && success == false){

        if(verbose){
          std::cout << std::endl;
          std::cout << "    Contacting" << std::endl;
          std::cout << "    " << url << std::endl;
        }

        CURL* curl = curl_easy_init();
  
        // Set remote URL.
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  
        // Don't bother trying IPv6, which would increase DNS resolution time.
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
  
        // Don't wait forever, time out after 20 seconds.
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT_TIME_SECONDS);
  
        // Follow HTTP redirects if necessary.
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  
        // Response information.
        long httpCode(0);
        std::unique_ptr<std::string> httpData(new std::string());
  
        // Hook up data handling function.
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callBack);
  
        // Hook up data container (will be passed as the last parameter to the
        // callBack handling function).  Can be any pointer type, since it will
        // internally be passed as a void pointer.
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());
  
        // Run our HTTP GET command, capture the HTTP response code, and clean up.
        unsigned int retryAttemptCount = 0;
  
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_easy_cleanup(curl);
  
        if(verbose){
          std::cout << "    http response code" << std::endl;
          std::cout << "    " << httpCode << std::endl;
        }

        if (httpCode == 200)
        {
          success=true;

          updUrlContents = (*httpData);
  

          if(verbose){    
            std::cout << "    Wrote https data to string" << std::endl;
          }
  
        }else{
          success=false;
        }
  
        ++downloadAttempts;
      }
  
    return success;
  };

};

#endif 