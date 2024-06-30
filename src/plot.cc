
#include <cstdio>
#include <fstream>
#include <string>
#include <cmath>
#include <algorithm>
#include <limits>

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

const double PointsPerInch       = 72;
const double InchesPerCentimeter  = 1.0/2.54;

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
  PlotSettings():
    fontName("Times"),
    axisLabelFontSize(8.0),
    axisTickFontSize(8.0),
    legendFontSize(6.0),
    titleFontSize(12.0),
    lineWidth(0.5),
    axisLineWidth(1.0),
    plotWidth(6.0*InchesPerCentimeter*PointsPerInch),
    plotHeight(6.0*InchesPerCentimeter*PointsPerInch),
    canvasWidth(0.),
    canvasHeight(0.){}  
};

//==============================================================================
//For now this assumes '%Y-%m-%d'
double convertToFractionalYear(std::string &dateStr){
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
void createHistoricalDataPlot(
      sciplot::Plot2D &plotHistoricalDataUpd,
      const nlohmann::ordered_json &reportEntry,
      const std::string &historicalFolder,
      const PlotSettings &settings,
      bool verbose){

  std::string primaryTicker;
  JsonFunctions::getJsonString(reportEntry["PrimaryTicker"],primaryTicker);

  std::string companyName;
  JsonFunctions::getJsonString(reportEntry["Name"],companyName);

  std::string currencyCode;
  JsonFunctions::getJsonString(reportEntry["CurrencyCode"],currencyCode);

  std::string country;
  JsonFunctions::getJsonString(reportEntry["Country"],country);

  //Load the historical data
  nlohmann::ordered_json historicalData;
  bool loadedHistoricalData = JsonFunctions::loadJsonFile(primaryTicker,
    historicalFolder, historicalData, verbose);  

  //Go through the historical data and pull out the date information
  //and adjusted_close information
  date::sys_days dateDay, dateDayFirstOfYear;
  date::year_month_day dateYmd;
  bool firstEntry = true;

  sciplot::Vec x0(historicalData.size());
  sciplot::Vec y0(historicalData.size());

  std::size_t dateCount=0;
  for( auto const &entry : historicalData){  
    y0[dateCount] = JsonFunctions::getJsonFloat(entry["adjusted_close"]);

    std::string dateStr;
    JsonFunctions::getJsonString(entry["date"],dateStr);
    x0[dateCount] = convertToFractionalYear(dateStr);

    ++dateCount;
    firstEntry=false;
  }

  plotHistoricalDataUpd.drawCurve(x0,y0)
      .label(companyName)
      .lineColor("black")
      .lineWidth(settings.lineWidth);

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

  for(auto const &entry: report){
    std::string primaryTickerEntry;
    JsonFunctions::getJsonString(entry["PrimaryTicker"],primaryTickerEntry);
    if(primaryTicker.compare(primaryTickerEntry.c_str())==0){
      std::string dateEntryStr;
      JsonFunctions::getJsonString(entry[dateFieldName],dateEntryStr);
      double timeData = convertToFractionalYear(dateEntryStr);
      dateSeries.push_back(timeData);

      double floatData = JsonFunctions::getJsonFloat(entry[floatFieldName]);
      floatSeries.push_back(floatData);
    }
  }

}

//==============================================================================
void plotReportData(
        nlohmann::ordered_json &report,
        std::string &historicalFolder,
        std::string &plotFolderOutput,
        PlotSettings &settings,
        bool verbose)
{


  for(auto const &reportEntry: report){



    sciplot::Plot2D plotHistoricalData;
    createHistoricalDataPlot( plotHistoricalData,reportEntry,historicalFolder, 
                              settings, verbose);

    std::string primaryTicker;
    JsonFunctions::getJsonString(reportEntry["PrimaryTicker"],primaryTicker);

    std::string companyName;
    JsonFunctions::getJsonString(reportEntry["Name"],companyName);

    std::string country;
    JsonFunctions::getJsonString(reportEntry["Country"],country);

    std::vector<double> xPriceToValue;
    std::vector<double> yPriceToValue;

    extractReportTimeSeriesData(
        primaryTicker,
        report,
        "dateEnd",
        "priceToValue_value",
        xPriceToValue,
        yPriceToValue);

    if(xPriceToValue.size()>1){
      std::reverse(xPriceToValue.begin(),xPriceToValue.end());
      std::reverse(yPriceToValue.begin(),yPriceToValue.end());
    }

    sciplot::Vec x(xPriceToValue.size());
    sciplot::Vec y(yPriceToValue.size());
    for(size_t i =0; i < yPriceToValue.size(); ++i){
      x[i] = xPriceToValue[i];
      y[i] = yPriceToValue[i];
    }

    sciplot::Plot2D plotPriceToValue;
    plotPriceToValue.drawCurve(x,y)
      .label(companyName)
      .lineColor("black")
      .lineWidth(settings.lineWidth);

    std::string tmpStringA("Year");
    std::string tmpStringB("Price-To-Value");
    configurePlot(plotPriceToValue,tmpStringA,tmpStringB,settings);
    plotPriceToValue.legend().atTopLeft();


    
    std::string titleStr = companyName;
    titleStr.append(" (");
    titleStr.append(primaryTicker);
    titleStr.append(") - ");
    titleStr.append(country);
    

    sciplot::Figure figTicker = {{plotHistoricalData, plotPriceToValue}};
    figTicker.title(titleStr);

    sciplot::Canvas canvas = {{figTicker}};
    canvas.size(settings.plotWidth*2.0,settings.plotHeight);
    //canvas.show();

    // Save the figure to a PDF file
    std::string outputFileName = plotFolderOutput;
    std::string plotFileName = primaryTicker;
    size_t idx = plotFileName.find(".");
    if(idx != std::string::npos){
      plotFileName.at(idx)='_';
    }
    plotFileName.append(".pdf");
    outputFileName.append(plotFileName);
    canvas.save(outputFileName);

    bool here=true;
  }

};

//==============================================================================
int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string historicalFolder;
  std::string reportFilePath;
  std::string plotFolder;

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
      "Analyze quarterly data. Caution: this is not yet been tested.", false);
    cmd.add(quarterlyAnalysisInput);    

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    exchangeCode              = exchangeCodeInput.getValue();    
    historicalFolder          = historicalFolderInput.getValue();    
    reportFilePath            = reportFilePathInput.getValue();
    plotFolder                = plotFolderOutput.getValue();
    analyzeQuarters           = quarterlyAnalysisInput.getValue();
    analyzeYears              = !analyzeQuarters;

    verbose             = verboseInput.getValue();

    if(verbose){
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

      std::cout << "  Historical Data Folder" << std::endl;
      std::cout << "    " << historicalFolder << std::endl;

      std::cout << "  Report File Path" << std::endl;
      std::cout << "    " << reportFilePath << std::endl;

      std::cout << "  Plot Folder" << std::endl;
      std::cout << "    " << plotFolder << std::endl;

      if(analyzeQuarters){
        std::cout << "Exiting: analyze quarters has not been tested "
                  << std::endl;
        std::abort();                  
      }
    }


    //Load the metric table data set
    nlohmann::ordered_json report;
    bool reportLoaded = 
      JsonFunctions::loadJsonFile(reportFilePath, report, verbose);

    PlotSettings settings;
          

    plotReportData(report,historicalFolder,plotFolder,settings,verbose);                                  

  } catch (TCLAP::ArgException &e){ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }


  return 0;
}
