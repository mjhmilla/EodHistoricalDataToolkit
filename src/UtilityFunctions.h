#ifndef UTILITY_FUNCTIONS
#define UTILITY_FUNCTIONS

#include <string>
#include <stdlib.h>

class UtilityFunctions {

public:

    static bool readListOfRankingMetrics(
              std::string& rankingFilePath,
              std::vector< std::vector< std::string > > &listOfRankingMetrics,
              bool verbose)
    {

      bool rankingFileIsValid=true;

        std::ifstream file(rankingFilePath);

        if(file.is_open()){      
          std::string entryA,entryB;
          do{        
            entryA.clear();
            std::getline(file,entryA,',');
            if(entryA.length() > 0){
              std::getline(file,entryB,'\n');
              std::vector< std::string> rankingMetric;          
              rankingMetric.push_back(entryA);
              rankingMetric.push_back(entryB);

              if(   entryB.compare("smallestIsBest") == 0
                 || entryB.compare("biggestIsBest" ) == 0){ 
                listOfRankingMetrics.push_back(rankingMetric);        
              }else{
                if(verbose){
                  std::cout << "Error: the second entry of each line in " 
                            << rankingFilePath 
                            << " must be either smallestIsBest or biggestIsBest. " 
                            << "This entry is not acceptable: " 
                            << entryB << std::endl; 
                }
              }

              if(rankingMetric.size() != 2){
                rankingFileIsValid=false;
                if(verbose){
                  std::cout << "Error: each line in " << rankingFilePath 
                            << " must have exactly 2 entries each separated "
                            << "by a comma." << std::endl; 
                }
              }
            }
          }while(entryA.length() > 0 && rankingFileIsValid);
          
          if(listOfRankingMetrics.size()==0){
            rankingFileIsValid=false;
            if(verbose){
              std::cout << "Error: " << rankingFilePath 
                        << " is empty. " << std::endl;
            }
          }

        }else{
          rankingFileIsValid = false;
          if(verbose){
            std::cout << "Error: could not open " << rankingFilePath << std::endl;
          }
        }

      return rankingFileIsValid;

    };



};

#endif

