#ifndef JSON_FUNCTIONS
#define JSON_FUNCTIONS

#include <string>
#include <nlohmann/json.hpp>
#include <stdlib.h>




class JsonFunctions {

  public:

    static bool loadJsonFile(const std::string &fileName, 
                             const std::string &folder,
                             nlohmann::ordered_json &jsonData,
                             bool verbose){

      bool success=true;
        
      try{
        //Load the json file                    
        std::stringstream ss;
        ss << folder << fileName;
        if(fileName.length() < 5){
          ss << ".json";
        }else if(fileName.substr(fileName.length()-5,5).compare(".json") != 0){
          ss << ".json";
        }
        std::string filePathName = ss.str();
        std::ifstream inputJsonFileStream(filePathName.c_str());
        jsonData = nlohmann::ordered_json::parse(inputJsonFileStream);
      }catch(const nlohmann::json::parse_error& e){
        std::cout << e.what() << std::endl;
        if(verbose){
          std::cout << "  Skipping: failed while reading json file" << std::endl; 
        }
        success=false;
      }  

      return success;
    };

    static double getJsonFloat(nlohmann::ordered_json &jsonEntry){
      if(  jsonEntry.is_null()){
        return std::nan("1");
      }else{
        if(jsonEntry.is_number_unsigned()){
          return static_cast<double>(jsonEntry.get<unsigned int>());
        }else if(jsonEntry.is_number_integer()){
          return static_cast<double>(jsonEntry.get<int>());
        }else if( jsonEntry.is_number_float()){
          return jsonEntry.get<double>();
        }else if (jsonEntry.is_string()){
          return std::atof(jsonEntry.get<std::string>().c_str());
        }else{
          throw std::invalid_argument("json entry is not a float or string");      
        }
      }
    };

    static bool getJsonBool(nlohmann::ordered_json &jsonEntry){
      if(  jsonEntry.is_null()){
        return std::nan("1");
      }else{
        if(  jsonEntry.is_boolean()){
          return jsonEntry.get<bool>();
        }else{
          throw std::invalid_argument("json entry is not a boolean");      
        }
      }
    };

    static void getJsonString(nlohmann::ordered_json &jsonEntry,
                              std::string &updString){
      if( jsonEntry.is_null()){
        updString="";
      }else{
        updString=jsonEntry.get<std::string>();
      }                            
    };

    static void getPrimaryTickerName(std::string &folder, 
                              std::string &fileName, 
                              std::string &updPrimaryTickerName){

      //Create the path and file name                          
      std::stringstream ss;
      ss << folder << fileName;
      std::string filePathName = ss.str();
      
      using json = nlohmann::ordered_json;
      std::ifstream jsonFileStream(filePathName.c_str());
      json jsonData = json::parse(jsonFileStream);  

      if( jsonData.contains("General") ){
        if(jsonData["General"].contains("PrimaryTicker")){
          if(jsonData["General"]["PrimaryTicker"].is_null() == false){
            updPrimaryTickerName = 
              jsonData["General"]["PrimaryTicker"].get<std::string>();
          }
        }
      }
    };


};

#endif