
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
const double InchesPerCentimeter = 1.0/2.54;


struct PlotSettings{
  std::string fontName;
  double axisLabelFontSize;
  double legendFontSize;
  double titleFontSize;  
  double lineWidth;
  double plotWidthInPoints;
  double plotHeighInPoints;
  double canvasWidthInPoints;
  double canvasHeightInPoints;
};


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

/*
void createStockPricePlot(
      std::string &primaryTicker,
      std::string &historicalFolder,
      std::string &companyName,
      std::string &currencyCode){

};
*/
void plotReportData(
        nlohmann::ordered_json &report,
        std::string &historicalFolder,
        std::string &plotFolderOutput,
        bool verbose)
{

  int plotWidthPts = 3*72;
  int plotHeightPts= 3*72;

  for(auto const &reportEntry: report){


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

    sciplot::Plot2D plot0, plot1, plot2, plot3, plot4, plot5;
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

    plot0.drawCurve(x0,y0).label(companyName).lineColor("black");
    plot0.xlabel("Year");
    std::string ylabelStr("Adjusted Closing Price (");
    ylabelStr.append(currencyCode);
    ylabelStr.append(")");

    plot0.ylabel(ylabelStr);
    plot0.size(plotWidthPts,plotHeightPts);
    plot0.fontName("Times");
    plot0.fontSize(12);

    //Using dummy data for now
    plot1.drawCurve(x0,y0).label(companyName).lineColor("black");
    plot1.xlabel("Time (days)");
    plot1.ylabel("Adjusted Closing Price ()");
    plot1.size(plotWidthPts,plotHeightPts);
    plot1.fontName("Times");
    plot1.fontSize(12);

    plot2.drawCurve(x0,y0).label(companyName).lineColor("black");
    plot2.xlabel("Time (days)");
    plot2.ylabel("Adjusted Closing Price ()");
    plot2.size(plotWidthPts,plotHeightPts);
    plot2.fontName("Times");
    plot2.fontSize(12);

    plot3.drawCurve(x0,y0).label(companyName).lineColor("black");
    plot3.xlabel("Time (days)");
    plot3.ylabel("Adjusted Closing Price ()");
    plot3.size(plotWidthPts,plotHeightPts);
    plot3.fontName("Times");
    plot3.fontSize(12);

    plot4.drawCurve(x0,y0).label(companyName).lineColor("black");
    plot4.xlabel("Time (days)");
    plot4.ylabel("Adjusted Closing Price ()");
    plot4.size(plotWidthPts,plotHeightPts);
    plot4.fontName("Times");
    plot4.fontSize(12);

    plot5.drawCurve(x0,y0).label(companyName).lineColor("black");
    plot5.xlabel("Time (days)");
    plot5.ylabel("Adjusted Closing Price ()");
    plot5.size(plotWidthPts,plotHeightPts);
    plot5.fontName("Times");
    plot5.fontSize(12);


    sciplot::Figure figTicker = {{plot0,plot1,plot2},{plot3,plot4,plot5}};

    std::string title = companyName;
    title.append(" (");
    title.append(primaryTicker);
    title.append(") - ");
    title.append(country);
    

    figTicker.title(title);//.fontName("Times").fontSize(12);
    //figTicker.fontName("Times");
    //figTicker.fontSize(12);

    figTicker.palette("dark2");

    sciplot::Canvas canvas = {{figTicker}};
    canvas.size(1366, 768);

    // Show the plot in a pop-up window
    canvas.show();

    // Save the figure to a PDF file
    std::string outputFileName = plotFolderOutput;
    std::string plotFileName = primaryTicker;
    size_t idx = plotFileName.find(".");
    if(idx != std::string::npos){
      plotFileName.at(idx)='_';
    }
    plotFileName.append(".png");
    outputFileName.append(plotFileName);
    canvas.save(outputFileName);

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

    plotReportData(report,historicalFolder,plotFolder,verbose);                                  

  } catch (TCLAP::ArgException &e){ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }


  return 0;
}
