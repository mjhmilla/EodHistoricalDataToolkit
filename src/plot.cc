
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
void escapeSpecialCharacters(std::string &textUpd, 
                            std::vector< char > &charactersToEscape){

    for(auto &escChar : charactersToEscape){
      size_t idx = textUpd.find(escChar,0);
      while(idx != std::string::npos){
        if(idx != std::string::npos){
          textUpd.insert(idx,"\\");
        }
        idx = textUpd.find(escChar,idx+2);
      }
    }
}
//==============================================================================
void convertCamelCaseSpacedText(std::string &labelUpd){
  //Convert a camel-case labelUpd to a spaced label                      
  labelUpd[0] = std::toupper(labelUpd[0]);
  
  for(size_t i=1;i<labelUpd.length();++i){
    if(std::islower(labelUpd[i-1]) && std::isupper(labelUpd[i])){
      labelUpd.insert(i," ");
    }            
    if(!std::isalnum(labelUpd[i])){
      labelUpd[i]=' ';
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
  double ymin = std::numeric_limits<double>::max();
  double ymax =-std::numeric_limits<double>::max();
  
  for( auto const &entry : historicalData){  
    y0[dateCount] = JsonFunctions::getJsonFloat(entry["adjusted_close"]);

    std::string dateStr;
    JsonFunctions::getJsonString(entry["date"],dateStr);
    x0[dateCount] = convertToFractionalYear(dateStr);

    if(static_cast<double>(y0[dateCount])<ymin){
      ymin=static_cast<double>(y0[dateCount]);
    }
    if(static_cast<double>(y0[dateCount])>ymax){
      ymax=static_cast<double>(y0[dateCount]);
    }

    ++dateCount;
    firstEntry=false;
  }

  plotHistoricalDataUpd.drawCurve(x0,y0)
      .label(companyName)
      .lineColor("black")
      .lineWidth(settings.lineWidth);

  if((ymax-ymin)>std::numeric_limits<double>::epsilon()){
    if(ymin > 0){
      ymin = 0;
      ymax=ymax + std::max(0.01,(ymax-ymin)*0.1);
    }
    if(ymax < 0){
      ymax=0;    
      ymin=ymin - std::max(0.01,(ymax-ymin)*0.1);
    }
    plotHistoricalDataUpd.yrange(static_cast<sciplot::StringOrDouble>(ymin),
                                static_cast<sciplot::StringOrDouble>(ymax));
  }
  

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
void plotReportData(
        const nlohmann::ordered_json &report,
        const std::string &historicalFolder,
        const std::string &plotFolderOutput,
        const std::vector< std::vector< std::string >> &subplotMetricNames,
        const PlotSettings &settings,
        std::vector< TickerFigurePath > &tickerFigurePath,
        int earliestReportingYear,
        int numberOfPlotsToGenerate,
        bool verbose)
{

  //Continue only if we have a plot configuration loaded.
  int entryCount=0;

  for(auto const &reportEntry: report){


    std::string primaryTicker;
    JsonFunctions::getJsonString(reportEntry["PrimaryTicker"],primaryTicker);

    std::string companyName;
    JsonFunctions::getJsonString(reportEntry["Name"],companyName);

    std::string country;
    JsonFunctions::getJsonString(reportEntry["Country"],country);

    std::string dateStart;
    JsonFunctions::getJsonString(reportEntry["dateStart"],dateStart);

    std::istringstream dateStartStream(dateStart);
    dateStartStream.exceptions(std::ios::failbit);
    date::year_month_day ymdStart;
    dateStartStream >> date::parse("%Y-%m-%d",ymdStart);
    int year = static_cast< int >(ymdStart.year());

    if( year >= earliestReportingYear){

    


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
            createHistoricalDataPlot( plotHistoricalData,reportEntry,
                                      historicalFolder,settings, verbose);          
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
            double ymin = std::numeric_limits<double>::max();
            double ymax = -std::numeric_limits<double>::max();
    
            for(size_t i =0; i < yTmp.size(); ++i){
              x[i] = xTmp[i];
              y[i] = yTmp[i];
              if(yTmp[i]>ymax){
                ymax=yTmp[i];
              }
              if(yTmp[i]<ymin){
                ymin=yTmp[i];
              }
            }

            sciplot::Plot2D plotMetric;
            plotMetric.drawCurve(x,y)
              .label(companyName)
              .lineColor("black")
              .lineWidth(settings.lineWidth);

            
            //if((ymax-ymin)>std::numeric_limits<double>::epsilon()){
            if(ymin > 0){
              ymin=0.;
              ymax=ymax + std::max(0.01,(ymax-ymin)*0.1);
            }
            if(ymax < 0){
              ymax=0.;
              ymin=ymin - std::max(0.01,(ymax-ymin)*0.1);
            }
            if((ymax-ymin)<0.01){
              ymin = -0.01;
              ymax = 0.01;
            }
            plotMetric.yrange(
                static_cast<sciplot::StringOrDouble>(ymin),
                static_cast<sciplot::StringOrDouble>(ymax));
          //} 
            

            std::string tmpStringA("Year");
            std::string tmpStringB(subplotMetricNames[indexRow][indexCol].c_str());

            size_t pos = tmpStringB.find("_value",0);
            tmpStringB = tmpStringB.substr(0,pos);

            convertCamelCaseSpacedText(tmpStringA);
            convertCamelCaseSpacedText(tmpStringB);

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
      //canvas.show();

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

      ++entryCount;

      if(entryCount > numberOfPlotsToGenerate && numberOfPlotsToGenerate > 0){
        break;
      }

      bool here=true;
    }
  }

};

void generateLaTeXReport(
    const nlohmann::ordered_json &report,
    const std::vector< TickerFigurePath > &tickerFigurePath,
    const std::string &plotFolder,
    const std::string &latexReportName,
    int earliestReportingYear,
    int numberOfPlotsToGenerate,
    bool verbose)
{
  std::vector< std::string > tabularMetrics;
  tabularMetrics.push_back("MarketCapitalizationMln");
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
  latexReport << "\\begin{document}"<<std::endl;
  
  std::vector< char > charactersToEscape;
  charactersToEscape.push_back('%');
  charactersToEscape.push_back('&');


  int entryCount=0;
  for(auto const &reportEntry: report){

    std::string primaryTicker;
    JsonFunctions::getJsonString(reportEntry["PrimaryTicker"],primaryTicker);

    std::string companyName;
    JsonFunctions::getJsonString(reportEntry["Name"],companyName);
    escapeSpecialCharacters(companyName,charactersToEscape);

    std::string country;
    JsonFunctions::getJsonString(reportEntry["Country"],country);

    std::string webURL;
    JsonFunctions::getJsonString(reportEntry["WebURL"],webURL);

    std::string description;
    JsonFunctions::getJsonString(reportEntry["Description"],description);
    escapeSpecialCharacters(description,charactersToEscape);



    std::string dateStart;
    JsonFunctions::getJsonString(reportEntry["dateStart"],dateStart);

    std::istringstream dateStartStream(dateStart);
    dateStartStream.exceptions(std::ios::failbit);
    date::year_month_day ymdStart;
    dateStartStream >> date::parse("%Y-%m-%d",ymdStart);
    int year = static_cast< int >(ymdStart.year());

    if(year >= earliestReportingYear){

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

      latexReport << description << std::endl;
      latexReport << std::endl;

      latexReport << "\\begin{center}" << std::endl;
      latexReport << "\\begin{tabular}{l l}" << std::endl;
      for(auto &entryMetric : tabularMetrics){
        double entryValue = JsonFunctions::getJsonFloat(reportEntry[entryMetric]);
        std::string labelMetric = entryMetric;
        convertCamelCaseSpacedText(labelMetric);
        latexReport << labelMetric << " & " << entryValue << "\\\\" << std::endl;
      }
      latexReport << "\\end{tabular}" << std::endl << std::endl;
      latexReport << "\\end{center}" << std::endl;


      latexReport << "\\break" << std::endl;
      latexReport << "\\newpage" << std::endl;

      ++entryCount;

      if(entryCount > numberOfPlotsToGenerate && numberOfPlotsToGenerate > 0){
        break;
      }
    }
  }
  

  latexReport << "\\end{document}" << std::endl;
  latexReport.close();

};

//==============================================================================
int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string historicalFolder;
  std::string reportFilePath;
  std::string plotFolder;
  std::string plotConfigurationFilePath;
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
      "Analyze quarterly data. Caution: this is not yet been tested.", false);
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
    numberOfPlotsToGenerate   = numberOfPlotsToGenerateInput.getValue();
    earliestReportingYear     = earliestReportingYearInput.getValue();
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
          
    std::vector< std::vector < std::string > > subplotMetricNames;

    readConfigurationFile(plotConfigurationFilePath,subplotMetricNames);

    std::vector< TickerFigurePath > tickerFigurePath;

    plotReportData(report,historicalFolder,plotFolder,subplotMetricNames,
                   settings,tickerFigurePath,earliestReportingYear, 
                   numberOfPlotsToGenerate,verbose);  

    std::string latexReportName 
      = reportFilePath.substr(reportFilePath.find_last_of("/\\")+1);
    latexReportName 
      = latexReportName.substr(0, latexReportName.find_last_of("."));
    latexReportName.append(".tex");

    generateLaTeXReport(report,
                        tickerFigurePath,
                        plotFolder,
                        latexReportName,
                        earliestReportingYear,
                        numberOfPlotsToGenerate,
                        verbose);

  } catch (TCLAP::ArgException &e){ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }


  return 0;
}
