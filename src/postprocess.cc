
#include <cstdio>
#include <fstream>
#include <string>
#include <cmath>
#include <algorithm>
#include <vector>
#include <numeric>
#include <limits>


#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include <sciplot/sciplot.hpp>

#include <filesystem>

#include "FinancialAnalysisToolkit.h"
#include "JsonFunctions.h"
#include "UtilityFunctions.h"
#include "ReportingFunctions.h"

const double PointsPerInch       = 72;
const double InchesPerCentimeter  = 1.0/2.54;

const double Percentiles[5]={0.05, 0.25, 0.5, 0.75, 0.95};

//==============================================================================
enum PercentileIndices{
  P05=0,
  P25,
  P50,
  P75,
  P95,
  NUM_PERCENTILES
};

enum ComparisonType{
  LESS_THAN=0,
  GREATER_THAN
};

struct MetricFilter{
  std::string name;
  double value;
  ComparisonType comparison;
  MetricFilter():
    name(""),
    value(0),
    comparison(LESS_THAN){};
};
//==============================================================================
struct SummaryStatistics{
  std::vector< double > percentiles;
  double min;
  double max;
  double current;
  std::string name;
  SummaryStatistics():
    min(0),
    max(0),
    current(0),
    name(""){};

};

//==============================================================================
struct TickerSummary{
  std::string ticker;
  unsigned int rank;
  double marketCapitalizationMln;
  std::vector<  SummaryStatistics > summaryStatistics;
  TickerSummary():
    ticker(""),
    rank(std::numeric_limits<unsigned int>::max()),
    marketCapitalizationMln(0){};
};

//==============================================================================
struct TickerFigurePath{
  std::string primaryTicker; 
  std::string figurePath;
};

//==============================================================================
struct PlotSettings{
  std::string fontName;
  double axisLabelFontSize;
  double axisTickFontSize;
  double legendFontSize;
  double titleFontSize;  
  double lineWidth;
  double axisLineWidth;
  double plotWidth;
  double plotHeight;
  double canvasWidth;
  double canvasHeight;
  double xticMinimumIncrement;
  PlotSettings():
    fontName("Times"),
    axisLabelFontSize(12.0),
    axisTickFontSize(12.0),
    legendFontSize(12.0),
    titleFontSize(14.0),
    lineWidth(1),
    axisLineWidth(1.0),
    plotWidth(8.0*InchesPerCentimeter*PointsPerInch),
    plotHeight(8.0*InchesPerCentimeter*PointsPerInch),
    canvasWidth(0.),
    canvasHeight(0.),
    xticMinimumIncrement(1.0){}  
};
//==============================================================================
//From:
//https://stackoverflow.com/questions/1577475/c-sorting-and-keeping-track-of-indexes
template <typename T>
std::vector< size_t > sort_indices(std::vector<T> &v){
  std::vector< size_t > idx(v.size());
  std::iota(idx.begin(),idx.end(),0);

  std::stable_sort(idx.begin(),idx.end(),
                    [&v](size_t i1, size_t i2){return v[i1]<v[i2];});
  std::stable_sort(v.begin(),v.end());

  return idx;
}
//==============================================================================
void readCountryFilterSet(const std::string &filterFilePath, 
                       std::vector< std::string > &listOfCountries){

  std::ifstream filterFile(filterFilePath);
  listOfCountries.clear();

  if(filterFile.is_open()){
    bool endOfFile=false;
    std::string line;
    do{
      line.clear();
      std::getline(filterFile,line);              
      if(line.length() > 0){
        listOfCountries.push_back(line);
      }else{
        endOfFile=true;
      }
    }while(!endOfFile);
  }
}

//==============================================================================
void readMetricValueFilterSet(const std::string &filterFilePath,
                           std::vector< MetricFilter > &metricFilterSet){
  std::ifstream filterFile(filterFilePath);
  metricFilterSet.clear();
  if(filterFile.is_open()){

    bool endOfFile=false;
    std::string line;

    while(!endOfFile){
      line.clear();

      std::getline(filterFile,line);

      if(line.length() > 0){
        MetricFilter metricFilter;

        size_t idx0=0;
        size_t idx1=0;

        idx1 = line.find_first_of(",",idx0);
        if(idx1 != std::string::npos){
          metricFilter.name = line.substr(idx0,idx1-idx0);
        }

        idx0=idx1+1;
        idx1 = line.find_last_of(",");
        if(idx1 != std::string::npos){
          std::string op = line.substr(idx0,idx1-idx0);
          if(op.compare("<")==0){
            metricFilter.comparison = ComparisonType::LESS_THAN;
          }else if(op.compare(">")==0){
            metricFilter.comparison = ComparisonType::GREATER_THAN;
          }else{
            std::cerr << "Error: readMetricValueFilterSet: Invalid operator of "
                      << op << " from file " << filterFilePath <<std::endl;
            std::abort();  
          }
        }
        ++idx1;
        double value = std::stod(line.substr(idx1, line.length()-idx1));
        metricFilter.value=value;

        metricFilterSet.push_back(metricFilter);
      }else{
        endOfFile=true;
      }      
    }
  }
}



//==============================================================================
void configurePlot(sciplot::Plot2D &plotUpd,
                   const std::string &xAxisLabel,
                   const std::string &yAxisLabel,
                   const PlotSettings &settings){

  plotUpd.xlabel(xAxisLabel)
    .fontName(settings.fontName)
    .fontSize(settings.axisLabelFontSize);

  plotUpd.xtics()
    .fontName(settings.fontName)
    .fontSize(settings.axisTickFontSize);

  
  plotUpd.ylabel(yAxisLabel)
    .fontName(settings.fontName)
    .fontSize(settings.axisLabelFontSize);

  plotUpd.ytics()
    .fontName(settings.fontName)
    .fontSize(settings.axisTickFontSize);

  plotUpd.border().lineWidth(settings.axisLineWidth);

  plotUpd.fontName(settings.fontName);
  plotUpd.fontSize(settings.legendFontSize);                    

  plotUpd.size(settings.plotWidth,settings.plotHeight);

};
//==============================================================================
void extractSummaryStatistics(sciplot::Vec &data, SummaryStatistics &summary){
  std::vector<double> dataDbl;
  for(size_t i=0; i<data.size(); ++i){
    dataDbl.push_back(data[i]);
  }
  summary.current = dataDbl[dataDbl.size()-1];

  std::sort(dataDbl.begin(),dataDbl.end());

  summary.min = dataDbl[0];
  summary.max = dataDbl[dataDbl.size()-1];


  if(dataDbl.size() > 1){
    for(size_t i = 0; i < NUM_PERCENTILES; ++i){
      double idxDbl = Percentiles[i]*(dataDbl.size()-1);
      int indexA = std::floor(idxDbl);
      int indexB = std::ceil(idxDbl);
      double weightB = idxDbl - static_cast<double>(indexA);
      double weightA = 1.0-weightB;
      double valueA = dataDbl[indexA];
      double valueB = dataDbl[indexB];
      double value = valueA*weightA + valueB*weightB;
      summary.percentiles.push_back(value);
    }
  }else{
    for(size_t i = 0; i < NUM_PERCENTILES; ++i){
      summary.percentiles.push_back(dataDbl[0]);
    }
  }

};



//==============================================================================
void createHistoricalDataPlot(
      sciplot::Plot2D &plotHistoricalDataUpd,
      const nlohmann::ordered_json &reportEntry,
      SummaryStatistics &summaryStatsUpd,
      const std::string &historicalFolder,
      const PlotSettings &settings,
      bool verbose){

  std::vector< char > charactersToDelete;  
  charactersToDelete.push_back('\'');

  std::string primaryTicker;
  JsonFunctions::getJsonString(reportEntry["PrimaryTicker"],primaryTicker);

  std::string companyName;
  JsonFunctions::getJsonString(reportEntry["Name"],companyName);
  UtilityFunctions::deleteCharacters(companyName,charactersToDelete);

  std::string currencyCode;
  JsonFunctions::getJsonString(reportEntry["CurrencyCode"],currencyCode);

  std::string country;
  JsonFunctions::getJsonString(reportEntry["Country"],country);

  //Load the historical data
  nlohmann::ordered_json historicalData;
  bool loadedHistoricalData =
            JsonFunctions::loadJsonFile(primaryTicker,
                                        historicalFolder, 
                                        historicalData, 
                                        verbose);  

  //Go through the historical data and pull out the date information
  //and adjusted_close information
  date::sys_days dateDay, dateDayFirstOfYear;
  date::year_month_day dateYmd;
  bool firstEntry = true;

  sciplot::Vec x0(historicalData.size());
  sciplot::Vec y0(historicalData.size());

  std::size_t dateCount=0;
  double ymin = std::numeric_limits<double>::max();
  double ymax =-std::numeric_limits<double>::max();
  double xmin = std::numeric_limits<double>::max();
  double xmax =-std::numeric_limits<double>::max();
  
  for( auto const &entry : historicalData){  
    y0[dateCount] = JsonFunctions::getJsonFloat(entry["adjusted_close"]);

    std::string dateStr;
    JsonFunctions::getJsonString(entry["date"],dateStr);
    x0[dateCount] = UtilityFunctions::convertToFractionalYear(dateStr);

    if(static_cast<double>(y0[dateCount])<ymin){
      ymin=static_cast<double>(y0[dateCount]);
    }
    if(static_cast<double>(y0[dateCount])>ymax){
      ymax=static_cast<double>(y0[dateCount]);
    }
    if(static_cast<double>(x0[dateCount])<xmin){
      xmin=static_cast<double>(x0[dateCount]);
    }
    if(static_cast<double>(x0[dateCount])>xmax){
      xmax=static_cast<double>(x0[dateCount]);
    }
    ++dateCount;
    firstEntry=false;
  }

  extractSummaryStatistics(y0,summaryStatsUpd);
  summaryStatsUpd.name = "historicalData";

  plotHistoricalDataUpd.drawCurve(x0,y0)
      .label(companyName)
      .lineColor("black")
      .lineWidth(settings.lineWidth);
    
  if((xmax-xmin)<1.0){
    xmax = 0.5*(xmin+xmax)+0.5;
    xmin = xmax-1.;
  }
  if((xmax-xmin)<5){
    plotHistoricalDataUpd.xtics().increment(settings.xticMinimumIncrement);
  }

  if((ymax-ymin)<0.1){
    ymax = 0.5*(ymin+ymax)+0.05;
    ymin = ymax-0.1;
  }

  if(ymin > 0){
    ymin=0;
  }
  if(ymax < 0){
    ymax=0;
  }

  ymax = ymax + 0.2*(ymax-ymin);

  plotHistoricalDataUpd.xrange(
      static_cast<sciplot::StringOrDouble>(xmin),
      static_cast<sciplot::StringOrDouble>(xmax));              

  plotHistoricalDataUpd.yrange(
      static_cast<sciplot::StringOrDouble>(ymin),
      static_cast<sciplot::StringOrDouble>(ymax));

  

  std::string xAxisLabel("Year");

  std::string yAxisLabel("Adjusted Closing Price (");
  yAxisLabel.append(currencyCode);
  yAxisLabel.append(")");

  configurePlot(plotHistoricalDataUpd,xAxisLabel,yAxisLabel,settings);

  plotHistoricalDataUpd.legend().atTopLeft();


};

//==============================================================================
void extractReportTimeSeriesData(
      const std::string &primaryTicker,
      const nlohmann::ordered_json &report,
      const char* dateFieldName,
      const char* floatFieldName,
      std::vector<double> &dateSeries,
      std::vector<double> &floatSeries){

  dateSeries.clear();
  floatSeries.clear();
  std::vector< double > tmpFloatSeries;

  for(auto const &entry: report){
    std::string primaryTickerEntry;
    JsonFunctions::getJsonString(entry["PrimaryTicker"],primaryTickerEntry);
    if(primaryTicker.compare(primaryTickerEntry.c_str())==0){
      std::string dateEntryStr;
      JsonFunctions::getJsonString(entry[dateFieldName],dateEntryStr);
      double timeData = UtilityFunctions::convertToFractionalYear(dateEntryStr);
      dateSeries.push_back(timeData);

      double floatData = JsonFunctions::getJsonFloat(entry[floatFieldName]);
      tmpFloatSeries.push_back(floatData);
    }
  }

  std::vector< size_t > indicesSorted = sort_indices(dateSeries);
  for(size_t i=0; i<indicesSorted.size();++i){
    floatSeries.push_back( tmpFloatSeries[ indicesSorted[i] ] );
  }

}
//==============================================================================
void readConfigurationFile(std::string &plotConfigurationFilePath,
                std::vector< std::vector < std::string > > &subplotMetricNames)
{

  std::ifstream configFile(plotConfigurationFilePath);
  if(configFile.is_open()){
    bool endOfFile=false;

      std::vector< std::string > metricRowVector; 
      std::string line;
      do{
        line.clear();
        metricRowVector.clear();

        std::getline(configFile,line);
        
        if(line.length() > 0){
          //Extract the individual fields
          size_t i0=0;
          size_t i1=line.find_first_of(',',i0);
          while(i1 > i0){
            std::string metricName = line.substr(i0,i1-i0);
            if(metricName.length()>0){
              metricRowVector.push_back(metricName);
              metricName.clear();
            }
            i0=i1+1;
            i1 = line.find_first_of(',',i0);
            if(i1 == std::string::npos){
              i1 = line.length();
            }            
          }
          if(metricRowVector.size() > 0){
            subplotMetricNames.push_back(metricRowVector);
          }
        }else{
          endOfFile=true;
        }
      }while(!endOfFile);
  }

};
//==============================================================================
void getDataRange(const std::vector< double > &data, 
                        std::vector< double > &dataRange,
                        double minimumRange){

  double ymin = std::numeric_limits<double>::max();
  double ymax = -std::numeric_limits<double>::max();

  for(size_t i =0; i < data.size(); ++i){
    if(data[i]>ymax){
      ymax=data[i];
    }
    if(data[i]<ymin){
      ymin=data[i];
    }
  }
  if(ymax-ymin < minimumRange){
    ymax = 0.5*(ymax+ymin)+0.5*minimumRange;
    ymin = ymax - minimumRange;
  }

  dataRange.clear();
  dataRange.push_back(ymin);
  dataRange.push_back(ymax);

}
//==============================================================================
int getTickerFigureNumber(const std::string &tickerName, 
                          std::vector< TickerFigurePath > &tickerFigurePath ){
  int tickerNumber=-1;

  if(tickerFigurePath.size()>0){
    tickerNumber=-1;
    for(int i=0; i< tickerFigurePath.size();++i){
      if(tickerName.compare(tickerFigurePath[i].primaryTicker)==0){
       tickerNumber=i;
      }
    }
  }
  return tickerNumber;
}
//==============================================================================
bool isTickerValid(const std::string &tickerName, 
                   const nlohmann::ordered_json &filterByTicker ){

  bool tickerIsValid = false;
  if(filterByTicker.size()>0){
    for(auto const &tickerEntry: filterByTicker){
      std::string tickerEntryStr(""); 
      JsonFunctions::getJsonString(tickerEntry, tickerEntryStr);
      if(tickerName.compare(tickerEntryStr)==0){
        tickerIsValid=true;
        break;
      }
    }                    
  }else{
    tickerIsValid=true;
  }

  return tickerIsValid;
}
//==============================================================================
bool isIndustryValid(const nlohmann::ordered_json &reportEntry, 
                   const nlohmann::ordered_json &filterByIndustry ){

  bool industryIsValid = false;

  if(filterByIndustry.size()>0){

    std::string reportGicSector(""), reportGicGroup(""), reportGicIndustry(""), 
                reportGicSubIndustry("");

    JsonFunctions::getJsonString(reportEntry["GicSector"],
                                 reportGicSector);            

    JsonFunctions::getJsonString(reportEntry["GicGroup"],
                                 reportGicGroup);            

    JsonFunctions::getJsonString(reportEntry["GicIndustry"],
                                 reportGicIndustry);

    JsonFunctions::getJsonString(reportEntry["GicSubIndustry"],
                                 reportGicSubIndustry);                    

    unsigned int includeCount = 0;
    unsigned int excludeCount = 0;
    bool includeFilter = false;
    bool excludeFilter = false;
    for(auto const &industryEntry: filterByIndustry){
      std::string gicSector(""), gicGroup(""), 
                  gicIndustry(""), gicSubIndustry(""), filterCommand("");
 
      JsonFunctions::getJsonString( industryEntry["Filter"],   
                                    filterCommand);

      if(industryEntry.contains("GicSector")){
        JsonFunctions::getJsonString( industryEntry["GicSector"], 
                                      gicSector);
      }
      if(industryEntry.contains("GicGroup")){
        JsonFunctions::getJsonString( industryEntry["GicGroup"],  
                                      gicGroup);
      }
      if(industryEntry.contains("GicIndustry")){
        JsonFunctions::getJsonString( industryEntry["GicIndustry"], 
                                      gicIndustry);
      }
      if(industryEntry.contains("GicSubIndustry")){
        JsonFunctions::getJsonString( industryEntry["GicSubIndustry"], 
                                      gicSubIndustry);
      }

      bool entryMatches=true;

      if(gicSector.length() > 0 && reportGicSector.length()>0){
        if(gicSector.compare(reportGicSector) !=0 ){
          entryMatches=false;
        }
      }
      if(gicGroup.length() > 0 && reportGicGroup.length()>0){
        if(gicGroup.compare(reportGicGroup) !=0 ){
          entryMatches=false;
        }       
      }
      if(gicIndustry.length() > 0 && reportGicIndustry.length()>0){
        if(gicIndustry.compare(reportGicIndustry) !=0 ){
          entryMatches=false;
        }               
      }
      if(gicSubIndustry.length() > 0 && reportGicSubIndustry.length()>0){
        if(gicSubIndustry.compare(reportGicSubIndustry) !=0 ){
          entryMatches=false;
        }                       
      }

      if(filterCommand.compare("Exclude")==0){
        if(entryMatches){
          ++excludeCount;
        }
        excludeFilter=true;
      }else if(filterCommand.compare("Include")==0){
        if(entryMatches){
          ++includeCount;
        }
        includeFilter=true;
      }else{
          std::cout << "  Error: filter_industry " 
                    << industryEntry.type_name() 
                    << " Filter should be Exclude/Include but is instead "
                    << filterCommand 
                    << std::endl;
      }
    }  

    if( !includeFilter && excludeFilter){
      if(excludeCount > 0){
        industryIsValid = false;
      }else{
        industryIsValid=true;
      }
    }

    if( includeFilter && !excludeFilter){
      if(includeCount > 0){
        industryIsValid = true;
      }else{
        industryIsValid=false;
      }
    }
    
    if(includeFilter && excludeFilter){
      if(includeCount > 0){
        industryIsValid=true;
      }
      if(excludeCount > 0){
        industryIsValid = false;
      }
    }


  }else{
    industryIsValid=true;
  }

  return industryIsValid;
}

//==============================================================================
bool isCountryValid( const std::string &country,
                     const std::vector< std::string > &filterByCountry){

  bool isValid = false;
  if(filterByCountry.size()>0){
    for(auto &countryName : filterByCountry){
      if(countryName.compare(country) == 0){
        isValid = true;
        break;
      }
    }
  }else{
    isValid=true;
  }
  return isValid;
};

//==============================================================================
bool areMetricsValid(const nlohmann::ordered_json &reportEntry,
                      const std::vector< MetricFilter > &metricFilterSet){

  bool isValid = true;
  if(metricFilterSet.size()>0){
    for(unsigned int i=0; i<metricFilterSet.size();++i){
      double value = JsonFunctions::getJsonFloat(
                      reportEntry[metricFilterSet[i].name]);
      switch(metricFilterSet[i].comparison){
        case ComparisonType::LESS_THAN : {
          if(value > metricFilterSet[i].value){
            isValid=false;
          }
        };
        break;
        case ComparisonType::GREATER_THAN : {
          if(value < metricFilterSet[i].value){
            isValid=false;
          }
        };
        break;
        default:
          std::cerr << "Error: areMetricsValid: invalid operator in "
                       "metricFilterSet" << std::endl;
          std::abort();                     
      };
      if(!isValid){
        break;
      }
    }
  }
  return isValid;
};
//==============================================================================
void drawBoxAndWhisker(
      sciplot::Plot2D &plotUpd,
      double x,   
      double xWidth,   
      const std::vector< double > &y,
      const char* lineColor,
      const PlotSettings &settings,
      bool verbose){

  sciplot::Vec xBox(5);
  sciplot::Vec yBox(5);

  xBox[0]= x-xWidth*0.5;
  xBox[1]= x+xWidth*0.5;
  xBox[2]= x+xWidth*0.5;
  xBox[3]= x-xWidth*0.5;
  xBox[4]= x-xWidth*0.5;

  yBox[0]= y[P25];
  yBox[1]= y[P25];
  yBox[2]= y[P75];
  yBox[3]= y[P75];
  yBox[4]= y[P25];

  plotUpd.drawCurve(xBox,yBox)
         .lineColor(lineColor)
         .lineWidth(settings.lineWidth);

  sciplot::Vec xLine(2);
  sciplot::Vec yLine(2);
  xLine[0] = x;
  xLine[1] = x;
  yLine[0] = y[P75];
  yLine[1] = y[P95];

  plotUpd.drawCurve(xLine,yLine)
         .lineColor(lineColor)
         .lineWidth(settings.lineWidth);
  
  xLine[0] = x;
  xLine[1] = x;
  yLine[0] = y[P25];
  yLine[1] = y[P05];

  plotUpd.drawCurve(xLine,yLine)
         .lineColor(lineColor)
         .lineWidth(settings.lineWidth);

  xLine[0] = x-xWidth*0.5;
  xLine[1] = x+xWidth*0.5;
  yLine[0] = y[P50];
  yLine[1] = y[P50];

  plotUpd.drawCurve(xLine,yLine)
         .lineColor(lineColor)
         .lineWidth(settings.lineWidth);


};
//==============================================================================
void plotReportData(
        const nlohmann::ordered_json &report,
        const std::string &historicalFolder,
        const std::string &plotFolderOutput,
        const std::vector< std::vector< std::string >> &subplotMetricNames,
        const PlotSettings &settings,
        const std::vector< std::string > &filterByCountry,
        const nlohmann::ordered_json &filterByIndustry,
        const nlohmann::ordered_json &filterByTicker,
        const std::vector< MetricFilter > &filterByMetricValue,
        int earliestReportingYear,
        int numberOfPlotsToGenerate,
        std::vector< TickerFigurePath > &tickerFigurePath,        
        bool verbose)
{

  if(verbose){
    std::cout << std::endl << "Generating plots" << std::endl << std::endl;

  }

  //Continue only if we have a plot configuration loaded.
  int entryCount=0;
  std::vector< char > charactersToDelete;  
  charactersToDelete.push_back('\'');

  std::vector< std::string > plottedTickers;

  std::vector < TickerSummary > marketSummary;

  for(auto const &reportEntry: report){


    std::string primaryTicker;
    JsonFunctions::getJsonString(reportEntry["PrimaryTicker"],primaryTicker);
    bool skip=false;

    if(!reportEntry.contains("Name")){
      if(verbose){
        std::cout << " Skipping " << primaryTicker << std::endl
                  << "    Entry is incomplete. Likely the corresponding "
                  << " historical data file does not exist or cannot be read. "
                  << std::endl << std::endl;
      }
      skip=true;
    }

    if(!skip){
      std::string companyName;
      JsonFunctions::getJsonString(reportEntry["Name"],companyName);
      UtilityFunctions::deleteCharacters(companyName,charactersToDelete);

      std::string country;
      JsonFunctions::getJsonString(reportEntry["Country"],country);

      std::string dateStart;
      JsonFunctions::getJsonString(reportEntry["dateStart"],dateStart);


      std::istringstream dateStartStream(dateStart);
      dateStartStream.exceptions(std::ios::failbit);
      date::year_month_day ymdStart;
      dateStartStream >> date::parse("%Y-%m-%d",ymdStart);
      int year = static_cast< int >(ymdStart.year());

      bool debuggingThisTicker=false;
      //if(primaryTicker.compare("DGE.LSE")==0){
      //  debuggingThisTicker=true;
      //}

      //Check to see if we have already plotted this ticker
      bool tickerIsNew=true;
      for(size_t i=0; i<plottedTickers.size();++i){
        if(plottedTickers[i].compare(primaryTicker) == 0){
          tickerIsNew=false;
          break;
        }
      }

      //Check if this data meets the criteria. Note: for this to work 
      //sensibily the report needs to be ordered from the most recent
      //date to the oldest;
      bool here=false;
      if(primaryTicker.compare("GOOG.US")==0){
        here=true;
      }

      bool tickerPassesFilter  = isTickerValid(primaryTicker,filterByTicker);    
      bool industryPassesFilter = isIndustryValid(reportEntry,filterByIndustry);
      bool countryPassesFilter = isCountryValid(country, filterByCountry);
      bool metricsPassFilter   = areMetricsValid(reportEntry,filterByMetricValue);
      bool entryAddedToReport=false;

      if( year >= earliestReportingYear && tickerIsNew 
          && countryPassesFilter && industryPassesFilter 
          && tickerPassesFilter && metricsPassFilter){

        TickerSummary tickerSummary;
        tickerSummary.ticker=primaryTicker;
        tickerSummary.rank= JsonFunctions::getJsonFloat( reportEntry["Ranking"]);
        tickerSummary.marketCapitalizationMln = 
            JsonFunctions::getJsonFloat( reportEntry["MarketCapitalizationMln"]);

        plottedTickers.push_back(primaryTicker);
        std::vector< std::vector < sciplot::PlotVariant > > arrayOfPlots;

        for(size_t indexRow=0; 
            indexRow < subplotMetricNames.size(); ++indexRow){

          std::vector < sciplot::PlotVariant > rowOfPlots;

          for(size_t indexCol=0; 
              indexCol < subplotMetricNames[indexRow].size(); ++indexCol){
            bool subplotAdded = false;

            //Add historical pricing data

            if(subplotMetricNames[indexRow][indexCol].compare("historicalData")==0){
              sciplot::Plot2D plotHistoricalData;
              SummaryStatistics priceSummaryStats;
              createHistoricalDataPlot( plotHistoricalData,reportEntry,
                                        priceSummaryStats,historicalFolder,
                                        settings, verbose);          
              tickerSummary.summaryStatistics.push_back(priceSummaryStats);

              rowOfPlots.push_back(plotHistoricalData);
              subplotAdded=true;
            }
            
            //Add metric data
            if(subplotMetricNames[indexRow][indexCol].compare("empty")!=0 
              && !subplotAdded){

              std::vector<double> xTmp;
              std::vector<double> yTmp;
              extractReportTimeSeriesData(
                  primaryTicker,
                  report,
                  "dateEnd",
                  subplotMetricNames[indexRow][indexCol].c_str(),
                  xTmp,
                  yTmp);     
              
              sciplot::Vec x(xTmp.size());
              sciplot::Vec y(yTmp.size());

              for(size_t i=0; i<xTmp.size();++i){
                x[i]=xTmp[i];
                y[i]=yTmp[i];
                if(debuggingThisTicker){
                  std::cout << x[i] << '\t' << y[i] << std::endl;
                }
              }
              SummaryStatistics metricSummaryStatistics;
              metricSummaryStatistics.name=subplotMetricNames[indexRow][indexCol];
              extractSummaryStatistics(y,metricSummaryStatistics);
              tickerSummary.summaryStatistics.push_back(metricSummaryStatistics);

              sciplot::Plot2D plotMetric;
              plotMetric.drawCurve(x,y)
                .label(companyName)
                .lineColor("black")
                .lineWidth(settings.lineWidth);

              std::vector< double > xRange,yRange;
              getDataRange(xTmp,xRange,1.0);
              getDataRange(yTmp,yRange,std::numeric_limits< double >::lowest());
              if(yRange[0] > 0){
                yRange[0] = 0;
              }
              if(yRange[1] < 0){
                yRange[1] = 0;
              }
              //Add some blank space to the top of the plot
              yRange[1] = yRange[1] + 0.2*(yRange[1]-yRange[0]);

              plotMetric.xrange(
                  static_cast<sciplot::StringOrDouble>(xRange[0]),
                  static_cast<sciplot::StringOrDouble>(xRange[1]));              

              plotMetric.yrange(
                  static_cast<sciplot::StringOrDouble>(yRange[0]),
                  static_cast<sciplot::StringOrDouble>(yRange[1]));

              if((xRange[1]-xRange[0])<5){
                plotMetric.xtics().increment(settings.xticMinimumIncrement);
              }

              std::string tmpStringA("Year");
              std::string tmpStringB(subplotMetricNames[indexRow][indexCol].c_str());

              size_t pos = tmpStringB.find("_value",0);
              tmpStringB = tmpStringB.substr(0,pos);

              UtilityFunctions::convertCamelCaseToSpacedText(tmpStringA);
              UtilityFunctions::convertCamelCaseToSpacedText(tmpStringB);

              configurePlot(plotMetric,tmpStringA,tmpStringB,settings);
              plotMetric.legend().atTopLeft();     
              rowOfPlots.push_back(plotMetric);
              subplotAdded=true;
            }

            //If its empty, do nothing.     
          }
          if(rowOfPlots.size()>0){
            arrayOfPlots.push_back(rowOfPlots);
          }
        }

        
        std::string titleStr = companyName;
        titleStr.append(" (");
        titleStr.append(primaryTicker);
        titleStr.append(") - ");
        titleStr.append(country);
        
        sciplot::Figure figTicker(arrayOfPlots);

        figTicker.title(titleStr);

        sciplot::Canvas canvas = {{figTicker}};
        unsigned int nrows = subplotMetricNames.size();
        unsigned int ncols=0;
        for(unsigned int i=0; i<subplotMetricNames.size();++i){
          if(ncols < subplotMetricNames[i].size()){
            ncols=subplotMetricNames[i].size();
          }
        }

        size_t canvasWidth  = 
          static_cast<size_t>(settings.plotWidth*static_cast<double>(ncols));
        size_t canvasHeight = 
          static_cast<size_t>(settings.plotHeight*static_cast<double>(nrows));

        canvas.size(canvasWidth, canvasHeight) ;

        if(debuggingThisTicker){
          canvas.show();
        }

        // Save the figure to a PDF file
        std::string outputFileName = plotFolderOutput;
        unsigned rank = 
          static_cast<unsigned int>(
              JsonFunctions::getJsonFloat(reportEntry["Ranking"]));
        std::ostringstream rankOSS;
        rankOSS << rank;          
        std::string rankStr(rankOSS.str());
        if(rankStr.length() < 5){
          rankStr.insert(0,5-rankStr.length(),'0');
        }
        rankStr.push_back('_');

        std::string plotFileName = rankStr;
        plotFileName.append(primaryTicker);
        size_t idx = plotFileName.find(".");
        if(idx != std::string::npos){
          plotFileName.at(idx)='_';
        }
        plotFileName.append(".pdf");
        outputFileName.append(plotFileName);
        canvas.save(outputFileName);

        TickerFigurePath entryTickerFigurePath;
        entryTickerFigurePath.primaryTicker = primaryTicker;
        entryTickerFigurePath.figurePath    = plotFileName;

        tickerFigurePath.push_back(entryTickerFigurePath);

        marketSummary.push_back(tickerSummary);
        ++entryCount;
        entryAddedToReport=true;




        if(entryCount > numberOfPlotsToGenerate && numberOfPlotsToGenerate > 0){
          break;
        }
        bool here=false;
        if(plotFileName.compare("00116_DGE_LSE.pdf")==0){
          here=true;
        }
      }
      if(verbose){
        if(entryAddedToReport){
          std::cout << entryCount << ". " << primaryTicker << std::endl;
        }else{
          std::cout << " Skipping " << primaryTicker << std::endl;

          if(!countryPassesFilter){
            std::cout <<" Filtered out by country" << std::endl;
          }
          if(!industryPassesFilter){
            std::cout <<" Filtered out by industry" << std::endl;
          }        
          if(!tickerPassesFilter){
            std::cout <<" Filtered out by ticker" << std::endl;
          }
          if(!metricsPassFilter){
            std::cout <<" Filtered out by metric" << std::endl;
          }
          if(!tickerIsNew){
            std::cout << " Already plotted " << primaryTicker << std::endl; 
          }
        }       
      }
    }
  }

  //Generate the Market SummaryPlot
  if(entryCount > 0){

    //To evaluate the summary statistics of the tickers in the report
    std::vector< SummaryStatistics > marketSummaryStatistics;
    double sumOfWeights = 0.;
    marketSummaryStatistics.resize(marketSummary[0].summaryStatistics.size());
    for(size_t i=0; i<marketSummaryStatistics.size();++i){
      marketSummaryStatistics[i].percentiles.resize(NUM_PERCENTILES);
      marketSummaryStatistics[i].name = 
        marketSummary[0].summaryStatistics[i].name;

      for(size_t j=0; j<NUM_PERCENTILES;++j){
        marketSummaryStatistics[i].percentiles[j]=0.;
      }
    }

    std::vector< std::vector < sciplot::Plot2D > > arrayOfPlot2D;
    arrayOfPlot2D.resize(subplotMetricNames.size());  

    for(size_t i=0; i<subplotMetricNames.size();++i){
      arrayOfPlot2D[i].resize(subplotMetricNames[i].size());          
    }



    double x=1;
    double xWidth = 0.4;
    for(auto const &entrySummary : marketSummary){      
      size_t indexMetric=0;  
      int indexTicker = getTickerFigureNumber(entrySummary.ticker,
                                              tickerFigurePath);
      if(indexTicker >= 0){
        x= static_cast<double>(indexTicker+1);
      }

      sumOfWeights += entrySummary.marketCapitalizationMln; 
      for(auto const &entryMetric : entrySummary.summaryStatistics){
        
        //Get the subplot row and column;
        size_t row=0;
        size_t col=0;
        for(size_t i=0; i<subplotMetricNames.size();++i){
          for(size_t j=0; j<subplotMetricNames[i].size();++j){
            if(entryMetric.name.compare(subplotMetricNames[i][j] )==0){
              row=i;
              col=j;
            }
          }
        }
        //Add the box and whisker plot
        drawBoxAndWhisker(
            arrayOfPlot2D[row][col],
            x,
            xWidth,
            entryMetric.percentiles,
            "gray",
            settings,
            verbose);
        //Draw the current value as a point
        sciplot::Vec xVec(2);
        sciplot::Vec yVec(2);
        xVec[0]=x-xWidth*0.5;
        xVec[1]=x+xWidth*0.5;
        yVec[0]=entryMetric.current;
        yVec[1]=entryMetric.current;
        arrayOfPlot2D[row][col].drawCurve(xVec,yVec)
                               .lineWidth(settings.lineWidth)
                               .lineColor("black");

        marketSummaryStatistics[indexMetric].current += 
          entryMetric.current*entrySummary.marketCapitalizationMln;
        marketSummaryStatistics[indexMetric].min += 
          entryMetric.min*entrySummary.marketCapitalizationMln;
        marketSummaryStatistics[indexMetric].max += 
          entryMetric.max*entrySummary.marketCapitalizationMln;
        for( size_t i = 0;  i< NUM_PERCENTILES;  ++i){
          marketSummaryStatistics[indexMetric].percentiles[i] +=
            entryMetric.percentiles[i]*entrySummary.marketCapitalizationMln;
        }

        ++indexMetric;                               
      }
      x=x+1.0;
    }

    //Normalize the market summary statistics
    for(size_t i=0; i < marketSummaryStatistics.size(); ++i){
      marketSummaryStatistics[i].min = 
        marketSummaryStatistics[i].min/sumOfWeights;
      marketSummaryStatistics[i].max = 
        marketSummaryStatistics[i].max/sumOfWeights;
      marketSummaryStatistics[i].current = 
        marketSummaryStatistics[i].current/sumOfWeights;

      for(size_t j=0; j<NUM_PERCENTILES; ++j){
        marketSummaryStatistics[i].percentiles[j]=
          marketSummaryStatistics[i].percentiles[j]/sumOfWeights;
      }
    }

    //Plot the market summary
    //Get the subplot row and column;
    for(auto const &entry : marketSummaryStatistics){
        size_t row=0;
        size_t col=0;
        for(size_t i=0; i<subplotMetricNames.size();++i){
          for(size_t j=0; j<subplotMetricNames[i].size();++j){
            if(entry.name.compare(
                              subplotMetricNames[i][j] )==0){
              row=i;
              col=j;
            }
          }
        }
        if(!filterByTicker.is_null()){
          x = static_cast<double>(tickerFigurePath.size())+1.0;
        }
        //Add the box and whisker plot
        drawBoxAndWhisker(
            arrayOfPlot2D[row][col],
            x,
            xWidth,
            entry.percentiles,
            "green",
            settings,
            verbose);
        //Draw the current value as a point
        sciplot::Vec xVec(2);
        sciplot::Vec yVec(2);
        xVec[0]=x-xWidth*0.5;
        xVec[1]=x+xWidth*0.5;
        yVec[0]=entry.current;
        yVec[1]=entry.current;
        arrayOfPlot2D[row][col].drawCurve(xVec,yVec)
                               .lineWidth(settings.lineWidth)
                               .lineColor("blue");

    }

    //Set the labels
    std::vector< std::vector < sciplot::PlotVariant > > arrayOfPlotVariant;

    for(size_t i=0; i<subplotMetricNames.size();++i){
      std::vector< sciplot::PlotVariant >  rowOfPlotVariant;
      for(size_t j=0; j<subplotMetricNames[i].size();++j){
        std::string tmpStringA("Reported Company Number");
        std::string tmpStringB(subplotMetricNames[i][j].c_str());

        size_t pos = tmpStringB.find("_value",0);
        tmpStringB = tmpStringB.substr(0,pos);

        UtilityFunctions::convertCamelCaseToSpacedText(tmpStringB);        
        configurePlot(arrayOfPlot2D[i][j],tmpStringA,tmpStringB,settings);
        arrayOfPlot2D[i][j].legend().hide();
        rowOfPlotVariant.push_back(arrayOfPlot2D[i][j]);
      }      
      arrayOfPlotVariant.push_back(rowOfPlotVariant);
    }

    sciplot::Figure figSummary(arrayOfPlotVariant);
    figSummary.title("Summary");
    sciplot::Canvas canvas = {{figSummary}};
    unsigned int nrows = subplotMetricNames.size();
    unsigned int ncols=0;
    for(unsigned int i=0; i<subplotMetricNames.size();++i){
      if(ncols < subplotMetricNames[i].size()){
        ncols=subplotMetricNames[i].size();
      }
    }

    size_t canvasWidth  = 
      static_cast<size_t>(settings.plotWidth*static_cast<double>(ncols));
    size_t canvasHeight = 
      static_cast<size_t>(settings.plotHeight*static_cast<double>(nrows));

    canvas.size(canvasWidth, canvasHeight) ;    

    // Save the figure to a PDF file
    std::string outputFileName = plotFolderOutput;
    std::string plotFileName = "summary.pdf";
    outputFileName.append(plotFileName);
    canvas.save(outputFileName);
  }

  if(verbose){
    std::cout << std::endl; 
  }
};


//==============================================================================
void generateLaTeXReport(
    const nlohmann::ordered_json &report,
    const std::vector< TickerFigurePath > &tickerFigurePath,
    const std::string &plotFolder,
    const std::string &analysisFolder,
    const std::vector< std::vector< std::string > > &subplotMetricNames,
    const std::string &latexReportName,
    const std::vector< std::string > &filterByCountry,
    const nlohmann::ordered_json &filterByIndustry,
    const nlohmann::ordered_json &filterByTicker,
    const std::vector< MetricFilter > &filterByMetricValue,    
    int earliestReportingYear,
    int numberOfPlotsToGenerate,
    bool verbose)
{

  if(verbose){
    std::cout << "Gerating LaTeX report" << std::endl; 
  }

  std::vector< std::string > plottedTickers;

  std::vector< std::string > tabularMetrics;
  tabularMetrics.push_back("MarketCapitalizationMln");
  tabularMetrics.push_back("Beta");
  tabularMetrics.push_back("IPODate");
  tabularMetrics.push_back("PercentInsiders");
  tabularMetrics.push_back("PercentInstitutions");
  tabularMetrics.push_back("AnalystRatings_TargetPrice");
  tabularMetrics.push_back("AnalystRatings_StrongBuy");
  tabularMetrics.push_back("AnalystRatings_Buy");
  tabularMetrics.push_back("AnalystRatings_Hold");
  tabularMetrics.push_back("AnalystRatings_Sell");
  tabularMetrics.push_back("AnalystRatings_StrongSell");


  std::string latexReportPath = plotFolder;
  latexReportPath.append(latexReportName);

  std::ofstream latexReport;
  latexReport.open(latexReportPath);

  //Append the opening latex commands
  latexReport << "\\documentclass[11pt,onecolumn,a4paper]{article}" 
              << std::endl;
  latexReport << "\\usepackage[hmargin={1.35cm,1.35cm},vmargin={2.0cm,3.0cm},"
                    "footskip=0.75cm,headsep=0.25cm]{geometry}"<<std::endl;
  latexReport << "\\usepackage{graphicx,caption}"<<std::endl;
  latexReport << "\\usepackage{times}"<<std::endl;
  latexReport << "\\usepackage{graphicx}"<<std::endl;
  latexReport << "\\usepackage{hyperref}"<<std::endl;
  latexReport << "\\usepackage{multicol}"<<std::endl;
  latexReport << "\\usepackage[usenames,dvipsnames,table]{xcolor}"<<std::endl;
  latexReport << "\\begin{document}"<<std::endl;
  
  std::vector< char > charactersToEscape;
  charactersToEscape.push_back('%');
  charactersToEscape.push_back('&');
  charactersToEscape.push_back('$');

  //Add the summary figure
  latexReport << "\\begin{figure}[h]" << std::endl;
  latexReport << "  \\begin{center}" << std::endl;
  latexReport << "    \\includegraphics{summary.pdf}" << std::endl;
  latexReport << "    \\caption{Summary statistics of the companies in this"
                    " report listed in order}" << std::endl;
  latexReport << " \\end{center}" << std::endl;
  latexReport << "\\end{figure}"<< std::endl;
  latexReport << std::endl;

  latexReport << "\\begin{multicols}{4}" << std::endl;
  latexReport << "\\begin{enumerate}" << std::endl;
  latexReport << "\\itemsep0pt" << std::endl;
  for(size_t i =0; i<tickerFigurePath.size(); ++i){
    latexReport << "\\item " <<  tickerFigurePath[i].primaryTicker << std::endl;
  }
  latexReport << "\\item Avg.Mkt.Cap" << std::endl;
  latexReport << "\\end{enumerate}" << std::endl;
  latexReport << std::endl;
  latexReport << "\\end{multicols}" << std::endl;


  latexReport << "\\break" << std::endl;
  latexReport << "\\newpage" << std::endl;

  int entryCount=0;
  for(auto const &reportEntry: report){

    std::string primaryTicker;
    JsonFunctions::getJsonString(reportEntry["PrimaryTicker"],primaryTicker);
    bool skip=false;

    if(!reportEntry.contains("Name")){
      if(verbose){
        std::cout << " Skipping " << primaryTicker << std::endl
                  << "    Entry is incomplete. Likely the corresponding "
                  << " historical data file does not exist or cannot be read. "
                  << std::endl << std::endl;
      }
      skip=true;
    }

    if(!skip){
      std::string companyName;
      JsonFunctions::getJsonString(reportEntry["Name"],companyName);
      UtilityFunctions::escapeSpecialCharacters(companyName,charactersToEscape);

      std::string country;
      JsonFunctions::getJsonString(reportEntry["Country"],country);

      std::string webURL;
      JsonFunctions::getJsonString(reportEntry["WebURL"],webURL);

      std::string description;
      JsonFunctions::getJsonString(reportEntry["Description"],description);
      UtilityFunctions::escapeSpecialCharacters(description,charactersToEscape);



      std::string dateStart;
      JsonFunctions::getJsonString(reportEntry["dateStart"],dateStart);

      std::istringstream dateStartStream(dateStart);
      dateStartStream.exceptions(std::ios::failbit);
      date::year_month_day ymdStart;
      dateStartStream >> date::parse("%Y-%m-%d",ymdStart);
      int year = static_cast< int >(ymdStart.year());

      //Check to see if we have already plotted this ticker
      bool tickerIsNew=true;
      for(size_t i=0; i<plottedTickers.size();++i){
        if(plottedTickers[i].compare(primaryTicker) == 0){
          tickerIsNew=false;
          break;
        }
      }

      bool tickerPassesFilter   = isTickerValid(primaryTicker,filterByTicker);
      bool industryPassesFilter = isIndustryValid(reportEntry, filterByIndustry);
      bool countryPassesFilter  = isCountryValid(country, filterByCountry);
      bool metricsPassFilter= areMetricsValid(reportEntry,filterByMetricValue);
      bool entryAddedToReport=false;

      if(year >= earliestReportingYear && tickerIsNew
          && countryPassesFilter && industryPassesFilter 
          && tickerPassesFilter && metricsPassFilter){

        plottedTickers.push_back(primaryTicker);

        size_t indexFigure=0;
        bool found=false;
        while(!found && indexFigure < tickerFigurePath.size()){
          if(tickerFigurePath[indexFigure].primaryTicker.compare(primaryTicker)==0){
            found=true;
          }else{
            ++indexFigure;
          }
        }

        latexReport << std::endl;

        latexReport << "\\begin{figure}[h]" << std::endl;
        latexReport << "  \\begin{center}" << std::endl;
        latexReport << "    \\includegraphics{" 
                    << tickerFigurePath[indexFigure].figurePath << "}" << std::endl;
        latexReport << "    \\caption{"
                    << companyName
                    << " (" << primaryTicker <<") "
                    << country << " ( \\url{" << webURL << "} )"
                    << "}" << std::endl;
        latexReport << " \\end{center}" << std::endl;
        latexReport << "\\end{figure}"<< std::endl;
        latexReport << std::endl;

        latexReport << "\\begin{multicols}{2}" << std::endl;

        latexReport << description << std::endl;
        latexReport << std::endl;

        //latexReport << "\\begin{center}" << std::endl;


        latexReport << "\\bigskip" << std::endl;
        latexReport << "\\begin{tabular}{l l}" << std::endl;
        latexReport << "\\multicolumn{2}{c}{\\textbf{Contextual Data}} \\\\" << std::endl;
        for(auto &entryMetric : tabularMetrics){
          double entryValue = JsonFunctions::getJsonFloat(reportEntry[entryMetric]);
          std::string labelMetric = entryMetric;
          UtilityFunctions::convertCamelCaseToSpacedText(labelMetric);
          latexReport << labelMetric << " & " << entryValue << "\\\\" << std::endl;
        }
        latexReport << "\\end{tabular}" << std::endl << std::endl;
        //latexReport << "\\end{center}" << std::endl;

        if(analysisFolder.length()>0){
          bool found=false;
          for(size_t i=0; i<subplotMetricNames.size(); ++i){
            for(size_t j=0; j<subplotMetricNames[i].size(); ++j){
              if(subplotMetricNames[i][j].compare("priceToValue_value")==0){
                ReportingFunctions::appendValuationTable(latexReport,
                                primaryTicker,analysisFolder,verbose);
              }
            }  
          }

          
        }

        latexReport << "\\end{multicols}" << std::endl;


        latexReport << "\\break" << std::endl;
        latexReport << "\\newpage" << std::endl;

        ++entryCount;
        entryAddedToReport=true;
        if(entryCount > numberOfPlotsToGenerate && numberOfPlotsToGenerate > 0){
          break;
        }
      }
      if(verbose){
        if(entryAddedToReport){
          std::cout << entryCount << ". " << primaryTicker << std::endl;
        }else{
          std::cout << " Skipping " << primaryTicker << std::endl;

          if(!countryPassesFilter){
            std::cout <<" Filtered out by country" << std::endl;
          }
          if(!industryPassesFilter){
            std::cout <<" Filtered out by industry" << std::endl;
          }
          if(!tickerPassesFilter){
            std::cout <<" Filtered out by ticker" << std::endl;
          }
          if(!metricsPassFilter){
            std::cout <<" Filtered out by metric" << std::endl;
          }
          if(!tickerIsNew){
            std::cout << " Already plotted " << primaryTicker << std::endl; 
          }
        }       
      }
    }

  }
  

  latexReport << "\\end{document}" << std::endl;
  latexReport.close();
    if(verbose){
      std::cout << std::endl;
    }
};

//==============================================================================
int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string historicalFolder;
  std::string reportFilePath;
  std::string analysisFolder;
  std::string plotFolder;
  std::string plotConfigurationFilePath;
  std::string fileFilterByTicker;
  std::string fileFilterByCountry;
  std::string fileFilterByIndustry;
  std::string fileFilterByMetricValue;
  std::string fileNameLaTexReport;

  int numberOfPlotsToGenerate = -1;
  int earliestReportingYear = 2023;
  bool analyzeYears=true;
  bool analyzeQuarters=false;

  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will produce reports in the form of text "
    "and tables about the companies that are best ranked."
    ,' ', "0.0");

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","exchange_code", 
      "The exchange code. For example: US",
      false,"","string");
    cmd.add(exchangeCodeInput);  

    TCLAP::ValueArg<std::string> reportFilePathInput("r","report_file_path", 
      "The path to the json report file.",
      true,"","string");
    cmd.add(reportFilePathInput);

    TCLAP::ValueArg<std::string> analysisFolderInput("a","analysis_calculation_file_path", 
      "The path to the folder that contains all of the analysis calculations.",
      true,"","string");
    cmd.add(analysisFolderInput);

    TCLAP::ValueArg<std::string> reportLatexFileNameInput("t","latex_report_file_name", 
      "The name of the LaTeX file to write.",
      true,"","string");
    cmd.add(reportLatexFileNameInput);

    TCLAP::ValueArg<std::string> fileFilterByTickerInput("s","filter_ticker", 
      "List of tickers to include as a json array",
      false,"","string");
    cmd.add(fileFilterByTickerInput); 

    TCLAP::ValueArg<std::string> fileFilterByCountryInput("l","filter_country", 
      "List of countries to include, listed one per line",
      false,"","string");
    cmd.add(fileFilterByCountryInput);    

    TCLAP::ValueArg<std::string> fileFilterByIndustryInput("g","filter_industry", 
      "Json file of the GIC (GicSector, GicGroup, GicIndustry, GicSubIndustry)"
      " categories of industries to include/exclude.",
      false,"","string");
    cmd.add(fileFilterByIndustryInput);    

    TCLAP::ValueArg<std::string> fileFilterByMetricValueInput("m","filter_metric_values", 
      "A csv file with 3 columns: metric name, operator (> or <), threshold"
      "value. Multiple rows can be added, but, each metric name must exist in"
      " the json report file.",
      false,"","string");
    cmd.add(fileFilterByMetricValueInput);    


    TCLAP::ValueArg<std::string> plotConfigurationFilePathInput("c",
      "configuration_file", 
      "The path to the csv file that contains the names of the metrics "
      "in the _report.json file to plot. Note that the keywords historicalData"
      "and empty are special: historicalData will produce a plot of the "
      "adjusted end-of-day stock price, and empty will result in an empty plot",
      true,"","string");
    cmd.add(plotConfigurationFilePathInput);

    TCLAP::ValueArg<std::string> historicalFolderInput("p",
      "historical_data_folder_path", 
      "The path to the folder that contains the historical (price)"
      " data json files from https://eodhistoricaldata.com/ to analyze",
      true,"","string");
    cmd.add(historicalFolderInput);

    TCLAP::ValueArg<std::string> plotFolderOutput("o","plot_folder_path", 
      "The path to the json report file.",
      true,"","string");
    cmd.add(plotFolderOutput);
    
    TCLAP::SwitchArg quarterlyAnalysisInput("q","quarterly",
      "Analyze quarterly data (TTM).", false);
    cmd.add(quarterlyAnalysisInput);    

    TCLAP::ValueArg numberOfPlotsToGenerateInput("n","number_of_firm_reports",
    "Number of firm reports to generate", true,-1,"int");
    cmd.add(numberOfPlotsToGenerateInput);

    TCLAP::ValueArg earliestReportingYearInput("y","earliest_reporting_year",
    "Include reports from this year to the present", true,2023,"int");
    cmd.add(earliestReportingYearInput);    

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    exchangeCode              = exchangeCodeInput.getValue();    
    plotConfigurationFilePath = plotConfigurationFilePathInput.getValue();
    historicalFolder          = historicalFolderInput.getValue();    
    reportFilePath            = reportFilePathInput.getValue();
    fileNameLaTexReport       = reportLatexFileNameInput.getValue();
    fileFilterByCountry       = fileFilterByCountryInput.getValue();
    fileFilterByMetricValue   = fileFilterByMetricValueInput.getValue();
    fileFilterByTicker        = fileFilterByTickerInput.getValue();
    fileFilterByIndustry      = fileFilterByIndustryInput.getValue();
    numberOfPlotsToGenerate   = numberOfPlotsToGenerateInput.getValue();
    earliestReportingYear     = earliestReportingYearInput.getValue();
    analysisFolder            = analysisFolderInput.getValue();
    plotFolder                = plotFolderOutput.getValue();
    analyzeQuarters           = quarterlyAnalysisInput.getValue();
    analyzeYears              = !analyzeQuarters;

    verbose             = verboseInput.getValue();

    if(verbose){
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

      std::cout << "  Include reports from this year to the present" << std::endl;
      std::cout << "    " << earliestReportingYear << std::endl;

      std::cout << "  Number of firms to analyze" << std::endl;
      std::cout << "    " << numberOfPlotsToGenerate << std::endl;

      std::cout << "  Plot Configuration File" << std::endl;
      std::cout << "    " << plotConfigurationFilePath << std::endl;

      std::cout << "  Country filter file" << std::endl;
      std::cout << "    " << fileFilterByCountry << std::endl;

      std::cout << "  Industry filter file" << std::endl;
      std::cout << "    " << fileFilterByIndustry << std::endl;

      std::cout << "  Ticker filter file" << std::endl;
      std::cout << "    " << fileFilterByTicker << std::endl;

      std::cout << "  Metric value filter file" << std::endl;
      std::cout << "    " << fileFilterByMetricValue << std::endl;

      std::cout << "  Historical Data Folder" << std::endl;
      std::cout << "    " << historicalFolder << std::endl;

      std::cout << "  Report File Path" << std::endl;
      std::cout << "    " << reportFilePath << std::endl;

      std::cout << "  Analysis Folder" << std::endl;
      std::cout << "    " << analysisFolder << std::endl;

      std::cout << "  Plot Folder" << std::endl;
      std::cout << "    " << plotFolder << std::endl;


    }



    //Load the metric table data set
    nlohmann::ordered_json report;
    bool reportLoaded = 
      JsonFunctions::loadJsonFile(reportFilePath, report, verbose);

    PlotSettings settings;
          
    std::vector< std::vector < std::string > > subplotMetricNames;

    readConfigurationFile(plotConfigurationFilePath,subplotMetricNames);

    std::vector< TickerFigurePath > tickerFigurePath;

    std::vector< std::string > filterByCountry;
    if(fileFilterByCountry.length()>0){
      readCountryFilterSet(fileFilterByCountry,filterByCountry);
    }

    std::vector< MetricFilter > filterByMetricValue;
    if(fileFilterByMetricValue.length()>0){
      readMetricValueFilterSet(fileFilterByMetricValue, filterByMetricValue);
    }

    nlohmann::ordered_json filterByTicker;
    if(fileFilterByTicker.length()>0){
      bool filterByTickerLoaded = 
        JsonFunctions::loadJsonFile(fileFilterByTicker, filterByTicker, 
                                    verbose);    
    }

    nlohmann::ordered_json filterByIndustry;
    if(fileFilterByIndustry.length()>0){
      bool filterByIndustryLoaded = 
        JsonFunctions::loadJsonFile(fileFilterByIndustry, filterByIndustry, 
                                    verbose);
    }

    plotReportData( report,
                    historicalFolder,
                    plotFolder,
                    subplotMetricNames,
                    settings,
                    filterByCountry,
                    filterByIndustry,
                    filterByTicker,
                    filterByMetricValue,
                    earliestReportingYear, 
                    numberOfPlotsToGenerate,
                    tickerFigurePath,
                    verbose);  
 
    generateLaTeXReport(report,
                        tickerFigurePath,
                        plotFolder,
                        analysisFolder,
                        subplotMetricNames,
                        fileNameLaTexReport,
                        filterByCountry,
                        filterByIndustry,
                        filterByTicker,
                        filterByMetricValue,                        
                        earliestReportingYear,
                        numberOfPlotsToGenerate,
                        verbose);

  } catch (TCLAP::ArgException &e){ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }


  return 0;
}
