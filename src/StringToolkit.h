#ifndef STRING_TOOLKIT
#define STRING_TOOLKIT

#include <string>
#include <stdlib.h>
#include <regex>

#include <boost/algorithm/string/classification.hpp> // Include boost::for is_any_of
#include <boost/algorithm/string/split.hpp> // Include for boost::split
#include <boost/algorithm/string.hpp> // Include for boost::iequals


class StringToolkit {

  public:

    static void removeFromString(std::string& str,
                   const std::string& removeStr)
    {
      std::string::size_type pos = 0u;
      pos = str.find(removeStr, pos);
      while(pos != std::string::npos){
         str.erase(pos, removeStr.length());
         pos += removeStr.length();
         pos = str.find(removeStr, pos);
      }
    };

    static void findAndReplaceString(std::string& str,
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
    };

    static void trim(std::string& str,
              const std::string& whitespace)
    {
        const auto strBegin = str.find_first_not_of(whitespace);
        if (strBegin != std::string::npos){
          const auto strEnd = str.find_last_not_of(whitespace);
          const auto strRange = strEnd - strBegin + 1;
          str = str.substr(strBegin, strRange);
        }
    
    };



    struct TextSimilarity{
      double score=0;
      bool firstWordsMatch;
      bool allWordsFound;
      bool exactMatch;
    };

    struct WordData{
      std::string raw;
      std::string lowercase;
      std::vector<std::string> words;
      double alphaLength;
      WordData(std::string& str){
        raw=str;
        lowercase=str;
        std::transform( lowercase.begin(),
                    lowercase.end(),
                    lowercase.begin(),
                    [](unsigned char c){return std::tolower(c);});

        //Remove extra additions that are not a part of any company name
        //Remove text between pairs of ( ) and - -
        lowercase = std::regex_replace(lowercase, 
                                std::regex("-.*-.*\\(.*\\)"), "");
        //Remove non-alphanumeric characters                                                
        lowercase = std::regex_replace(lowercase, 
                                std::regex("[^a-zA-Z\\d\\s:]"), "");
        //Trim leading and following whitespace
        StringToolkit::trim(lowercase," ");        

        //Break the company name into an std::vector of words
        boost::split(words, lowercase, boost::is_any_of(" "), 
                    boost::token_compress_on);
        alphaLength= std::count_if(
                        lowercase.begin(),
                        lowercase.end(),
                        [](unsigned char c){
                            return std::isalpha(c);});

      };
    };


    struct TextData{
      std::string raw;
      std::string lowercase;
      std::vector<std::string> words;
      double alphaLength;
      TextData(std::string& str){
        raw=str;
        lowercase=str;
        std::transform( lowercase.begin(),
                    lowercase.end(),
                    lowercase.begin(),
                    [](unsigned char c){return std::tolower(c);});
        alphaLength= std::count_if(
                        lowercase.begin(),
                        lowercase.end(),
                        [](unsigned char c){
                            return std::isalpha(c);});

      }
    };


    static void evaluateSimilarity( WordData &textA,
                                    TextData &textB,
                                    TextSimilarity &result)
    {

      double scoreA=0.;
      double scoreB=0.;                          

      //Evaluate the similarity score
      if(textA.raw.compare(textB.raw)==0){
        scoreA = 1.0;
        scoreB = 1.0;
        result.exactMatch=true;
        
      }else if(textB.raw.find(textA.raw) != std::string::npos){
        result.allWordsFound = true;
        scoreA = ( textA.alphaLength / textA.alphaLength );
        scoreB = ( textA.alphaLength / textB.alphaLength );  

      }else if(textA.raw.find(textB.raw) != std::string::npos){
        result.allWordsFound = true;
        scoreA = ( textB.alphaLength / textA.alphaLength );
        scoreB = ( textB.alphaLength / textB.alphaLength );  

      }else{

        size_t pos =0;
        size_t loc = 0;
        unsigned int lengthOfMatchingWords=0;            
        result.allWordsFound = true;

        for(unsigned int i=0; i < textA.words.size();++i){
          pos = 0;
          bool wordValid = false;
          while(loc != std::string::npos && wordValid==false){

            loc = textB.lowercase.find(textA.words[i],pos);                
            pos += loc + textA.words[i].length();

            if(loc != std::string::npos){
              char prev = ' ';
              if(i>0){
                prev = textB.lowercase[loc-1];
              }
              char next = ' ';
              if(loc+textA.words[i].length()<textB.lowercase.length()){
                  next = textB.lowercase[loc+textA.words[i].length()];
              }                  
              if( !std::isalpha(prev) && !std::isalpha(next)){
                wordValid=true;
              }
            }
          }
          if(!wordValid){
            result.allWordsFound=false;
          }
          if(wordValid){                
            double wordLength = textA.words[i].length();
            scoreA+= ( wordLength / textA.alphaLength );
            scoreB+= ( wordLength / textB.alphaLength );
            if(i==0 && loc == 0){
              result.firstWordsMatch=true;
            }
          }              
        }
        
      }

      result.score = std::min(scoreA,scoreB);
      
    };    

};



#endif