#ifndef JSON_FUNCTIONS
#define JSON_FUNCTIONS

#include <string>
#include <nlohmann/json.hpp>
#include <stdlib.h>
#include <numeric>




class JsonFunctions {

  public:

    //When an accounting value is missing this number is inserted to 
    //signal that it is missing. This value has been chosen:
    //
    //1. To be less than 1 dollar/Eur/Yen etc. Why? Fractions of a dollar/Eur/Yen
    //   never (?) in financial statements. Seeing this show up in a table later 
    //   will cause the reader to wonder about it. This also means that this value
    //   can be used as a signal for a missing value later.
    //
    //2. It rounds to zero. 
    //
    static constexpr double MISSING_VALUE = 0.000001;


    static bool loadJsonFile(const std::string &fileName, 
                             const std::string &folder,
                             nlohmann::ordered_json &jsonData,
                             bool verbose){
       std::stringstream ss;
       ss << folder << fileName;                              
       return loadJsonFile(ss.str(),jsonData,verbose);

    }

    static bool loadJsonFile(const std::string &fullFilePath,
                             nlohmann::ordered_json &jsonData,
                             bool verbose){

      bool success=true;
      std::string filePath = fullFilePath;
      try{
        //Load the json file                           
        if(filePath.length() < 5){
          filePath.append(".json");
        }else if(filePath.substr(filePath.length()-5,5).compare(".json") != 0){
          filePath.append(".json");
        }
        std::ifstream inputJsonFileStream(filePath.c_str());
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

    static bool isJsonFloatValid(double value){
      if(std::isnan(value) || std::isinf(value)){
        return false;
      }else if(std::abs(value-MISSING_VALUE) < 
                std::numeric_limits< double >::epsilon()*10.0){
        return false;
      }else{
        return true;
      }
    }

    static bool doesFieldExist(const nlohmann::ordered_json &jsonTable,
                               std::vector< std::string > &fields){
      bool fieldExists = true;                                
      for(size_t i=0; i<fields.size();++i){
        switch(i){
          case 0:{
            fieldExists = jsonTable.contains(fields[0]);
          }break;
          case 1:{
            if(fieldExists){
              fieldExists = jsonTable[fields[0]].contains(fields[1]);
            }
          }break;          
          case 2:{
            if(fieldExists){
              fieldExists = jsonTable[fields[0]][fields[1]].contains(fields[2]);
            }
          }break;          
          case 3:{
            if(fieldExists){
              fieldExists = jsonTable[fields[0]][fields[1]]
                                                [fields[2]].contains(fields[3]);
            }
          }break;          
          case 4:{
            if(fieldExists){
              fieldExists = jsonTable[fields[0]][fields[1]]
                                     [fields[2]][fields[3]].contains(fields[4]);
            }
          }break;        
          default:
            fieldExists=false;  
        }
      }
      return fieldExists;
    };

    static bool getJsonBool(const nlohmann::ordered_json &jsonTable,
                               std::vector< std::string > &fields,
                               bool setNansToMissingValue=false){

      bool value = 0;
    
      switch( fields.size() ){
        case 1:{
            value = getJsonBool(
                      jsonTable[fields[0]],
                      setNansToMissingValue);
          }
          break;
        case 2:{
            value = getJsonBool(
                      jsonTable[fields[0]][fields[1]],
                      setNansToMissingValue);
          }
          break;
        case 3:{
            value = getJsonBool(
                      jsonTable[fields[0]][fields[1]][fields[2]],
                      setNansToMissingValue);
          }
          break;
        case 4:{
            value = getJsonBool(
                      jsonTable[fields[0]][fields[1]][fields[2]][fields[3]],
                      setNansToMissingValue);
          }
          break;
        default: {
          if(setNansToMissingValue){
            value = false;
          }else{
            value = std::nan("1");
          }          
        }
      };            

      return value;
    };

    static double getJsonFloat(const nlohmann::ordered_json &jsonTable,
                               std::vector< std::string > &fields,
                               bool setNansToMissingValue=false){

      double value = 0;
    
      switch( fields.size() ){
        case 1:{
            bool valid = true;
            value = getJsonFloat(
                      jsonTable[fields[0]],
                      setNansToMissingValue);
          }
          break;
        case 2:{
            value = getJsonFloat(
                      jsonTable[fields[0]][fields[1]],
                      setNansToMissingValue);
          }
          break;
        case 3:{
            value = getJsonFloat(
                      jsonTable[fields[0]][fields[1]][fields[2]],
                      setNansToMissingValue);
          }
          break;
        case 4:{
            value = getJsonFloat(
                      jsonTable[fields[0]][fields[1]][fields[2]][fields[3]],
                      setNansToMissingValue);
          }
          break;
        default: {
          if(setNansToMissingValue){
            value = MISSING_VALUE;
          }else{
            value = std::nan("1");
          }          
        }
      };            

      return value;
    };

    static void getJsonString(const nlohmann::ordered_json &jsonTable,
                               std::vector< std::string > &fields,
                               std::string &stringUpd){
    
      switch( fields.size() ){
        case 1:{
            getJsonString(  jsonTable[fields[0]],
                            stringUpd);
          }
          break;
        case 2:{
            getJsonString(  jsonTable[fields[0]][fields[1]],
                            stringUpd);
          }
          break;
        case 3:{
            getJsonString(  jsonTable[fields[0]][fields[1]][fields[2]],
                            stringUpd);
          }
          break;
        case 4:{
            getJsonString(  jsonTable[fields[0]][fields[1]][fields[2]][fields[3]],
                            stringUpd);
          }
          break;
        default: {
          stringUpd = "";          
        }
      };            
    };    

    static double getJsonFloat(const nlohmann::ordered_json &jsonEntry,
                               bool setNansToMissingValue=false){
      if(  jsonEntry.is_null()){
        if(setNansToMissingValue){
          return MISSING_VALUE;
        }else{
          return std::nan("1");
        }
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


    static bool getJsonBool(const nlohmann::ordered_json &jsonEntry,
                            bool replaceNanWithFalse=false){
      if(  jsonEntry.is_null()){
        if(replaceNanWithFalse){
          return false;
        }else{
          return std::nan("1");
        }
      }else{
        if(  jsonEntry.is_boolean()){
          return jsonEntry.get<bool>();
        }else{
          throw std::invalid_argument("json entry is not a boolean");      
        }
      }
    };

    static void getJsonString(const nlohmann::ordered_json &jsonEntry,
                              std::string &updString){
      if( jsonEntry.is_null()){
        updString="";
      }else{
        updString=jsonEntry.get<std::string>();
      }                            
    };

    static void getPrimaryTickerName(const std::string &folder, 
                              const std::string &fileName, 
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