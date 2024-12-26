#ifndef UTILITY_FUNCTIONS
#define UTILITY_FUNCTIONS

#include <string>
#include <stdlib.h>
#include <chrono>
#include "date.h"

class UtilityFunctions {

public:

//==============================================================================
static double convertToFractionalYear(const std::string &dateStr){
  date::year_month_day dateYmd;
  date::sys_days dateDay, dateDayFirstOfYear;

  std::istringstream dateStrStreamA(dateStr);
  dateStrStreamA.exceptions(std::ios::failbit);
  dateStrStreamA >> date::parse("%Y-%m-%d",dateDay);

  std::istringstream dateStrStreamB(dateStr);
  dateStrStreamB.exceptions(std::ios::failbit);
  dateStrStreamB >> date::parse("%Y-%m-%d",dateYmd);

  int year = int(dateYmd.year());
  std::string dateStrFirstOfYear(dateStr.substr(0,4));
  dateStrFirstOfYear.append("-01-01");

  std::istringstream dateStrStreamC(dateStrFirstOfYear);
  dateStrStreamC.exceptions(std::ios::failbit);      
  dateStrStreamC >> date::parse("%Y-%m-%d",dateDayFirstOfYear);

  double daysInYear = 365.0;
  if(dateYmd.year().is_leap()){
    daysInYear=364.0;
  }
  double date = double(year) 
    + double( (dateDay-dateDayFirstOfYear).count() )/daysInYear;

  return date;
};

//==============================================================================
static bool readListOfRankingMetrics(
  const std::string& rankingFilePath,
  std::vector< std::vector< std::string > > &listOfRankingMetricsUpd,
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
          listOfRankingMetricsUpd.push_back(rankingMetric);        
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
    
    if(listOfRankingMetricsUpd.size()==0){
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

