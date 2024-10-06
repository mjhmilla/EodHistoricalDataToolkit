
#include <cstdio>
#include <fstream>
#include <string>
#include <cmath>
#include <algorithm>
#include <limits>
#include <filesystem>

#include <chrono>
#include "date.h"
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

const double PointsPerInch        = 72;
const double InchesPerCentimeter  = 1.0/2.54;
const double Percentiles[5]       ={0.05, 0.25, 0.5, 0.75, 0.95};

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
enum PercentileIndices{
  P05=0,
  P25,
  P50,
  P75,
  P95,
  NUM_PERCENTILES
};

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
void createHistoricalDataPlot(
      const nlohmann::ordered_json &historicalData,
      const std::string &companyName,
      const std::string &currencyCode,
      sciplot::Plot2D &plotHistoricalDataUpd,      
      SummaryStatistics &summaryStatsUpd,
      const PlotSettings &settings,
      bool verbose){

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
      const nlohmann::ordered_json &calculateData,
      const char* dateFieldName,
      const char* floatFieldName,
      std::vector<double> &dateSeries,
      std::vector<double> &floatSeries){

  dateSeries.clear();
  floatSeries.clear();
  std::vector< double > tmpFloatSeries;

  for(auto const &entry: calculateData){
    std::string dateEntryStr;
    JsonFunctions::getJsonString(entry[dateFieldName],dateEntryStr);
    double timeData = UtilityFunctions::convertToFractionalYear(dateEntryStr);
    dateSeries.push_back(timeData);

    double floatData = JsonFunctions::getJsonFloat(entry[floatFieldName]);
    tmpFloatSeries.push_back(floatData);
    
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
void sanitizeStringForLaTeX(std::string &stringForLatex){

  std::vector< char > charactersToDelete;  
  charactersToDelete.push_back('\'');
  UtilityFunctions::deleteCharacters(stringForLatex,charactersToDelete);

  std::vector< char > charactersToEscape;
  charactersToDelete.push_back('&');
  charactersToDelete.push_back('$');
  charactersToDelete.push_back('#');

  UtilityFunctions::escapeSpecialCharacters(stringForLatex, charactersToEscape);


}
//==============================================================================
void plotTickerData(
    const nlohmann::ordered_json &fundamentalData,
    const nlohmann::ordered_json &historicalData,
    const nlohmann::ordered_json &calculateData,
    const std::filesystem::path &outputPath,
    const std::vector< std::vector< std::string >> &subplotMetricNames,
    const PlotSettings &plotSettings,
    bool verbose)
{


  std::string primaryTicker;
  JsonFunctions::getJsonString( fundamentalData[GEN]["PrimaryTicker"],
                                primaryTicker);

  std::string companyName;
  JsonFunctions::getJsonString( fundamentalData[GEN]["Name"],
                                companyName);
  sanitizeStringForLaTeX(companyName);                                

  std::string currencyCode;
  JsonFunctions::getJsonString( fundamentalData[GEN]["CurrencyCode"],
                                currencyCode);
  sanitizeStringForLaTeX(currencyCode);


  for(size_t indexRow=0; indexRow < subplotMetricNames.size(); 
      ++indexRow){
    std::vector < sciplot::PlotVariant > rowOfPlots;

    for(size_t indexCol=0; indexCol < subplotMetricNames[indexRow].size(); 
        ++indexCol){
      bool subplotAdded = false; 

      if(subplotMetricNames[indexRow][indexCol].compare("historicalData")==0){
        sciplot::Plot2D plotHistoricalData;
        SummaryStatistics priceSummaryStats;
        createHistoricalDataPlot( historicalData,
                                  companyName,
                                  currencyCode,
                                  plotHistoricalData,
                                  priceSummaryStats,
                                  plotSettings, 
                                  verbose);    
        //tickerSummary.summaryStatistics.push_back(priceSummaryStats);

        rowOfPlots.push_back(plotHistoricalData);
        subplotAdded=true;
      }      

      if(subplotMetricNames[indexRow][indexCol].compare("empty")!=0 
        && !subplotAdded){
          std::vector<double> xTmp;
          std::vector<double> yTmp;
          extractReportTimeSeriesData(
              primaryTicker,
              calculateData,
              "dateEnd",
              subplotMetricNames[indexRow][indexCol].c_str(),
              xTmp,
              yTmp);     
          
          sciplot::Vec x(xTmp.size());
          sciplot::Vec y(yTmp.size());

          for(size_t i=0; i<xTmp.size();++i){
            x[i]=xTmp[i];
            y[i]=yTmp[i];
          }
          SummaryStatistics metricSummaryStatistics;
          metricSummaryStatistics.name=subplotMetricNames[indexRow][indexCol];
          extractSummaryStatistics(y,metricSummaryStatistics);

          sciplot::Plot2D plotMetric;
          plotMetric.drawCurve(x,y)
            .label(companyName)
            .lineColor("black")
            .lineWidth(plotSettings.lineWidth);

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
            plotMetric.xtics().increment(plotSettings.xticMinimumIncrement);
          }

          std::string tmpStringA("Year");
          std::string tmpStringB(subplotMetricNames[indexRow][indexCol].c_str());

          size_t pos = tmpStringB.find("_value",0);
          tmpStringB = tmpStringB.substr(0,pos);

          UtilityFunctions::convertCamelCaseToSpacedText(tmpStringA);
          UtilityFunctions::convertCamelCaseToSpacedText(tmpStringB);

          configurePlot(plotMetric,tmpStringA,tmpStringB,plotSettings);
          plotMetric.legend().atTopLeft();     
          rowOfPlots.push_back(plotMetric);
          subplotAdded=true;        
      }      


    }
  }

};

//==============================================================================
int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string calculateDataFolder;
  std::string fundamentalFolder;
  std::string historicalFolder;
  std::string plotConfigurationFilePath;  
  std::string reportFolder;

  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will produce reports in the form of text "
    "and tables about the companies that are best ranked."
    ,' ', "0.0");

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","exchange_code", 
      "The exchange code. For example: US",
      false,"","string");
    cmd.add(exchangeCodeInput);  

    TCLAP::ValueArg<std::string> reportFolderOutput("o","report_folder_path", 
      "The path to the folder will contain the reports.",
      true,"","string");
    cmd.add(reportFolderOutput);

    TCLAP::ValueArg<std::string> calculateDataFolderInput("a","calculate_folder_path", 
      "The path to the folder contains the output "
      "produced by the calculate method",
      true,"","string");
    cmd.add(calculateDataFolderInput);

    TCLAP::ValueArg<std::string> fundamentalFolderInput("f",
      "fundamental_data_folder_path", 
      "The path to the folder that contains the fundamental data json files from "
      "https://eodhistoricaldata.com/ to analyze",
      true,"","string");
    cmd.add(fundamentalFolderInput);

    TCLAP::ValueArg<std::string> historicalFolderInput("p",
      "historical_data_folder_path", 
      "The path to the folder that contains the historical (price)"
      " data json files from https://eodhistoricaldata.com/ to analyze",
      true,"","string");
    cmd.add(historicalFolderInput);

    TCLAP::ValueArg<std::string> plotConfigurationFilePathInput("c",
      "configuration_file", 
      "The path to the csv file that contains the names of the metrics "
      " to plot. Note that the keywords historicalData"
      "and empty are special: historicalData will produce a plot of the "
      "adjusted end-of-day stock price, and empty will result in an empty plot",
      true,"","string");
    cmd.add(plotConfigurationFilePathInput);    

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    exchangeCode              = exchangeCodeInput.getValue();  
    plotConfigurationFilePath = plotConfigurationFilePathInput.getValue();      
    calculateDataFolder       = calculateDataFolderInput.getValue();
    fundamentalFolder         = fundamentalFolderInput.getValue();
    historicalFolder          = historicalFolderInput.getValue();    
    reportFolder              = reportFolderOutput.getValue();
    verbose                   = verboseInput.getValue();

    if(verbose){
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

      std::cout << "  Calculate Data Input Folder" << std::endl;
      std::cout << "    " << calculateDataFolder << std::endl;

      std::cout << "  Fundamental Data Folder" << std::endl;
      std::cout << "    " << fundamentalFolder << std::endl;

      std::cout << "  Historical Data Folder" << std::endl;
      std::cout << "    " << historicalFolder << std::endl;

      std::cout << "  Report Folder" << std::endl;
      std::cout << "    " << reportFolder << std::endl;  

      std::cout << "  Plot Configuration File" << std::endl;
      std::cout << "    " << plotConfigurationFilePath << std::endl;          
    }


  PlotSettings plotSettings;
  std::vector< std::vector < std::string > > subplotMetricNames;
  readConfigurationFile(plotConfigurationFilePath,subplotMetricNames);

  int totalFileCount = 0;
  int validFileCount = 0;
  std::string analysisExt = ".json";  

  //Go through every ticker file in the calculate folder:
  //1. Create a ticker folder in the reporting folder
  //2. Save to this ticker folder:
  //  a. The pdf of the ticker's plots
  //  b. The LaTeX file for the report (as an input)
  //  c. A wrapper LaTeX file to generate an individual report

  for (const auto & file 
        : std::filesystem::directory_iterator(calculateDataFolder)){  

    ++totalFileCount;
    bool validInput = true;

    //
    // Check the file name
    //    
    std::string fileName   = file.path().filename(); 
    std::size_t fileExtPos = fileName.find(analysisExt);

    if(verbose){
      std::cout << totalFileCount << '\t' << fileName << std::endl;
    }    

    if( fileExtPos == std::string::npos ){
      validInput=false;
      if(verbose){
        std::cout << '\t' 
                  << "Skipping " << fileName 
                  << " should end in .json but this does not:" << std::endl; 
      }
    }


    if(validInput){


      //
      // Create the output folder
      //
      std::string ticker = fileName.substr(0,fileExtPos);
      std::string tickerFolderName = ticker;
      std::replace(tickerFolderName.begin(),tickerFolderName.end(),'.','_');

      std::string outputFolderName = reportFolderOutput;
      outputFolderName.append(tickerFolderName);

      std::filesystem::path outputFolderPath = outputFolderName;
      if( !std::filesystem::is_directory(outputFolderPath) ){
        std::filesystem::create_directory(outputFolderPath);
      }


      //
      //Read in the inputs
      //
      nlohmann::ordered_json fundamentalData;  
      bool loadedFundData =JsonFunctions::loadJsonFile(ticker, 
                                fundamentalFolder, fundamentalData, verbose);
                                  
      nlohmann::ordered_json historicalData;
      bool loadedHistData =JsonFunctions::loadJsonFile(ticker, 
                                historicalFolder, historicalData, verbose); 
                                
      nlohmann::ordered_json calculateData;
      bool loadedCalculateData =JsonFunctions::loadJsonFile(ticker, 
                                calculateDataFolder, calculateData, verbose);


      if(loadedFundData && loadedHistData && loadedCalculateData){
        nlohmann::ordered_json tickerReportDataUpd;
        //
        // Generate and save the pdf
        //
        plotTickerData(
            fundamentalData,
            historicalData,
            calculateData,
            outputFolderPath,
            subplotMetricNames,
            plotSettings,
            verbose);

        ++validFileCount;     
      }


    }


  }
 
  if(verbose){
    std::cout << std::endl;
    std::cout << "Summary" << std::endl;
    std::cout << '\t' << totalFileCount 
              << '\t' << "number of tickers processed" << std::endl;
    std::cout << '\t' << validFileCount
              << '\t' << "number of tickers with valid data" << std::endl;
  } 



  } catch (TCLAP::ArgException &e){ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }


  return 0;
}
