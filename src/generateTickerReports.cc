
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
#include "PlottingFunctions.h"


const char* BoxAndWhiskerColorA="web-blue";
const char* BoxAndWhiskerColorB="gray0";



//==============================================================================
struct TickerMetaData{
  std::string primaryTicker;
  std::string companyName;
  std::string currencyCode;
  std::string country;
  std::string isin;
  TickerMetaData():
    primaryTicker(""),
    companyName(""),
    currencyCode(""),
    country(""),
    isin(""){}
};
//==============================================================================
enum JSON_FIELD_TYPE{
  FLOAT=0,
  STRING,
  BOOL,
  JSON_FIELD_TYPE_SIZE
};

struct JsonFieldAddress{
  std::vector< std::string > fieldNames;
  JSON_FIELD_TYPE type;
  JsonFieldAddress():fieldNames(),type(FLOAT){};
};

//==============================================================================
bool isNameInList(std::string &name, std::vector< std::string > &listOfNames){
  bool nameIsInList = false;
  size_t i = 0;
  while(nameIsInList == false && i < listOfNames.size()){
    if(name.compare(listOfNames[i])==0){
      nameIsInList=true;
    }
    ++i;
  }
  return nameIsInList;
};

//==============================================================================
void sanitizeStringForLaTeX(std::string &stringForLatex){

  std::string charactersToDelete("");  
  charactersToDelete.append("\'");
  UtilityFunctions::deleteCharacters(stringForLatex,charactersToDelete);

  std::string charactersToEscape("");
  charactersToEscape.append("&");
  charactersToEscape.append("$");
  charactersToEscape.append("#");

  UtilityFunctions::escapeSpecialCharacters(stringForLatex, charactersToEscape);


}
//==============================================================================
void getTickerMetaData(    
    const nlohmann::ordered_json &fundamentalData,
    TickerMetaData &tickerMetaDataUpd){

  JsonFunctions::getJsonString( fundamentalData[GEN]["PrimaryTicker"],
                                tickerMetaDataUpd.primaryTicker);

  JsonFunctions::getJsonString( fundamentalData[GEN]["Name"],
                                tickerMetaDataUpd.companyName);                                
  sanitizeStringForLaTeX(tickerMetaDataUpd.companyName);                                

  JsonFunctions::getJsonString( fundamentalData[GEN]["ISIN"],
                                tickerMetaDataUpd.isin);                                

  JsonFunctions::getJsonString( fundamentalData[GEN]["CurrencyCode"],
                                tickerMetaDataUpd.currencyCode);
  sanitizeStringForLaTeX(tickerMetaDataUpd.currencyCode);

  JsonFunctions::getJsonString(fundamentalData[GEN]["CountryName"],
                               tickerMetaDataUpd.country);  
  sanitizeStringForLaTeX(tickerMetaDataUpd.country);      

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
void createHistoricalDataPlot(
      const nlohmann::ordered_json &historicalData,
      const std::string &companyName,
      const std::string &currencyCode,
      sciplot::Plot2D &plotHistoricalDataUpd,      
      PlottingFunctions::SummaryStatistics &summaryStatsUpd,
      const PlottingFunctions::PlotSettings &settings,
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

  PlottingFunctions::extractSummaryStatistics(y0,summaryStatsUpd);
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

  plotHistoricalDataUpd.legend().atTopLeft();  

  PlottingFunctions::drawBoxAndWhisker(
      plotHistoricalDataUpd,
      xmax+1,
      0.5,
      summaryStatsUpd,
      BoxAndWhiskerColorA,
      BoxAndWhiskerColorB,
      settings,
      verbose);
  xmax += 2.0;

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

  PlottingFunctions::configurePlot(plotHistoricalDataUpd,xAxisLabel,yAxisLabel,
                                  settings);



};

//==============================================================================
void extractTimeSeriesData(
      const std::string &primaryTicker,
      const nlohmann::ordered_json &calculateData,
      const char* floatFieldName,
      std::vector<double> &dateSeries,
      std::vector<double> &floatSeries){

  dateSeries.clear();
  floatSeries.clear();
  std::vector< double > tmpFloatSeries;

  for(auto const &el: calculateData.items()){
    
    std::string dateEntryStr(el.key());
    double timeData = UtilityFunctions::convertToFractionalYear(dateEntryStr);
    dateSeries.push_back(timeData);

    double floatData = 
      JsonFunctions::getJsonFloat(calculateData[el.key()][floatFieldName]);
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
bool plotTickerData(
    const TickerMetaData &tickerMetaData,
    const nlohmann::ordered_json &fundamentalData,
    const nlohmann::ordered_json &historicalData,
    const nlohmann::ordered_json &calculateData,
    const char* outputPlotPath,
    const std::vector< std::vector< std::string >> &subplotMetricNames,
    const PlottingFunctions::PlotSettings &plotSettings,
    bool verbose)
{
  bool success = true;               

  std::vector< std::vector < sciplot::PlotVariant > > arrayOfPlots;

  for(size_t indexRow=0; indexRow < subplotMetricNames.size(); 
      ++indexRow){

    std::vector < sciplot::PlotVariant > rowOfPlots;

    for(size_t indexCol=0; indexCol < subplotMetricNames[indexRow].size(); 
        ++indexCol){
      bool subplotAdded = false; 

      if(subplotMetricNames[indexRow][indexCol].compare("historicalData")==0){
        sciplot::Plot2D plotHistoricalData;
        PlottingFunctions::SummaryStatistics priceSummaryStats;
        createHistoricalDataPlot( historicalData,
                                  tickerMetaData.companyName,
                                  tickerMetaData.currencyCode,
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
          extractTimeSeriesData(
              tickerMetaData.primaryTicker,
              calculateData,
              subplotMetricNames[indexRow][indexCol].c_str(),
              xTmp,
              yTmp);     
          
          sciplot::Vec x(xTmp.size());
          sciplot::Vec y(yTmp.size());

          for(size_t i=0; i<xTmp.size();++i){
            x[i]=xTmp[i];
            y[i]=yTmp[i];
          }
          PlottingFunctions::SummaryStatistics metricSummaryStatistics;
          metricSummaryStatistics.name=subplotMetricNames[indexRow][indexCol];
          PlottingFunctions::extractSummaryStatistics(y,metricSummaryStatistics);

          sciplot::Plot2D plotMetric;
          plotMetric.drawCurve(x,y)
            .label(tickerMetaData.companyName)
            .lineColor("black")
            .lineWidth(plotSettings.lineWidth);

          std::vector< double > xRange,yRange;
          PlottingFunctions::getDataRange(xTmp,xRange,1.0);
          PlottingFunctions::getDataRange(yTmp,yRange,
                              std::numeric_limits< double >::lowest());
          if(yRange[0] > 0){
            yRange[0] = 0;
          }
          if(yRange[1] < 0){
            yRange[1] = 0;
          }
          //Add some blank space to the top of the plot
          yRange[1] = yRange[1] + 0.2*(yRange[1]-yRange[0]);

          plotMetric.legend().atTopLeft();   

          PlottingFunctions::drawBoxAndWhisker(
              plotMetric,
              xRange[1]+1,
              0.5,
              metricSummaryStatistics,
              BoxAndWhiskerColorA,
              BoxAndWhiskerColorB,
              plotSettings,
              verbose);

          xRange[1] += 2.0;

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

          PlottingFunctions::configurePlot(plotMetric,tmpStringA,tmpStringB,
                                           plotSettings);
            
          rowOfPlots.push_back(plotMetric);
          subplotAdded=true;        
      }      
    }
    if(rowOfPlots.size()>0){
      arrayOfPlots.push_back(rowOfPlots);
    }
  }

  std::string titleStr = tickerMetaData.companyName;
  titleStr.append(" (");
  titleStr.append(tickerMetaData.primaryTicker);
  titleStr.append(") - ");
  titleStr.append(tickerMetaData.country);
  
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
    static_cast<size_t>(plotSettings.plotWidth*static_cast<double>(ncols));
  size_t canvasHeight = 
    static_cast<size_t>(plotSettings.plotHeight*static_cast<double>(nrows));

  canvas.size(canvasWidth, canvasHeight) ;

  // Save the figure to a PDF file
  canvas.save(outputPlotPath);

  return success;

};
//==============================================================================
bool generateLaTeXReportWrapper(
    const char* wrapperFilePath,
    const std::string &reportFileName,
    const TickerMetaData &tickerMetaData,
    bool verbose)
{
  bool success = true;
  if(verbose){
    std::cout << "Gerating LaTeX report wrapper file for " 
              << tickerMetaData.primaryTicker  << std::endl << std::endl; 
  }


  std::ofstream latexReport;
  try{
    latexReport.open(wrapperFilePath);
  }catch(std::ofstream::failure &ofstreamErr){
    std::cerr << std::endl << std::endl
              << "Skipping writing " << tickerMetaData.primaryTicker              
              << " because an exception was thrown while trying to open ofstream:"
              << wrapperFilePath << std::endl << std::endl
              << ofstreamErr.what()
              << std::endl;    
    success = false;
  }

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
  latexReport << "\\input{" << reportFileName << "}" << std::endl;
  latexReport << "\\end{document}" << std::endl;
  latexReport.close();

  return success;
};

//==============================================================================
bool generateLaTeXReport(
    const TickerMetaData &tickerMetaData,
    const nlohmann::ordered_json &fundamentalData,
    const nlohmann::ordered_json &historicalData,
    const nlohmann::ordered_json &calculateData,
    const char* plotFileName,
    const char* outputReportPath,
    const std::vector< std::vector< std::string >> &subplotMetricNames,
    bool replaceNansWithMissingData,
    bool verbose)
{

  bool skip = false;

  if(verbose){
    std::cout << "Gerating LaTeX report for " 
              << tickerMetaData.primaryTicker  << std::endl; 
  }

  std::vector< JsonFieldAddress > tabularMetrics;
  JsonFieldAddress metric;

  metric.fieldNames.push_back("General");
  metric.fieldNames.push_back("ISIN");
  metric.type = JSON_FIELD_TYPE::STRING;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("General");
  metric.fieldNames.push_back("CurrencyCode");
  metric.type = JSON_FIELD_TYPE::STRING;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("General");
  metric.fieldNames.push_back("IPODate");
  metric.type = JSON_FIELD_TYPE::STRING;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("Highlights");
  metric.fieldNames.push_back("MarketCapitalizationMln");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);
  
  metric.fieldNames.clear();
  metric.fieldNames.push_back("Technicals");
  metric.fieldNames.push_back("Beta");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("Valuation");
  metric.fieldNames.push_back("EnterpriseValueEbitda");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("SharesStats");
  metric.fieldNames.push_back("PercentInsiders");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("SharesStats");
  metric.fieldNames.push_back("PercentInstitutions");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("TargetPrice");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("StrongBuy");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("Buy");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("Hold");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("Sell");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  metric.fieldNames.clear();
  metric.fieldNames.push_back("AnalystRatings");
  metric.fieldNames.push_back("StrongSell");
  metric.type = JSON_FIELD_TYPE::FLOAT;
  tabularMetrics.push_back(metric);

  std::string latexReportPath(outputReportPath);
  std::ofstream latexReport;

  try{
    latexReport.open(latexReportPath);
  }catch(std::ofstream::failure &ofstreamErr){
    std::cerr << std::endl << std::endl 
              << "Skipping writing " << latexReportPath              
              << " because an exception was thrown while trying to open ofstream:"
              << latexReportPath << std::endl << std::endl
              << ofstreamErr.what()
              << std::endl;    
    skip=true;
  }


  if(!skip){

    std::string webURL;
    JsonFunctions::getJsonString(fundamentalData[GEN]["WebURL"],webURL);

    std::string description;
    JsonFunctions::getJsonString(
        fundamentalData[GEN]["Description"],description);
    sanitizeStringForLaTeX(description);

    latexReport << std::endl;

    latexReport << "\\begin{figure}[h]" << std::endl;
    latexReport << "  \\begin{center}" << std::endl;
    latexReport << "    \\includegraphics{" 
                <<      plotFileName << "}" << std::endl;
    latexReport << "    \\caption{"
                << tickerMetaData.companyName
                << " (" << tickerMetaData.primaryTicker <<") "
                << tickerMetaData.country << " ( \\url{" << webURL << "} )"
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

    for(size_t  indexTabularMetrics=0; 
                indexTabularMetrics < tabularMetrics.size();
              ++indexTabularMetrics)
    {
      JsonFieldAddress entryMetric = tabularMetrics[indexTabularMetrics];
      std::string labelMetric = entryMetric.fieldNames.back();
      UtilityFunctions::convertCamelCaseToSpacedText(labelMetric);

      bool fieldExists = JsonFunctions::doesFieldExist(fundamentalData, 
                                                entryMetric.fieldNames);

      if(fieldExists){
        switch(entryMetric.type){
          case JSON_FIELD_TYPE::BOOL : {
            bool value = JsonFunctions::getJsonBool(fundamentalData,
                              entryMetric.fieldNames, 
                              replaceNansWithMissingData);

            latexReport << labelMetric << " & " << value  << "\\\\" << std::endl;                          
          }
          break;
          case JSON_FIELD_TYPE::FLOAT : {
            double value = JsonFunctions::getJsonFloat(fundamentalData,
                            entryMetric.fieldNames, 
                            replaceNansWithMissingData);

            latexReport << labelMetric << " & " << value << "\\\\" << std::endl;                          
          }
          break;
          case JSON_FIELD_TYPE::STRING : {
            std::string value;
            JsonFunctions::getJsonString(fundamentalData,
                              entryMetric.fieldNames, 
                              value);

            latexReport << labelMetric << " & " << value << "\\\\" << std::endl;                          
          }
          break;
        };
      }else{
        latexReport << labelMetric << " & " << " - "  << "\\\\" << std::endl;
      }
    }
    latexReport << "\\end{tabular}" << std::endl << std::endl;

    std::string date = calculateData.begin().key();

    for(size_t i=0; i<subplotMetricNames.size(); ++i){
      for(size_t j=0; j<subplotMetricNames[i].size(); ++j){
        if(subplotMetricNames[i][j].compare("priceToValue")==0){
          ReportingFunctions::appendValuationTable(
            latexReport,
            tickerMetaData.primaryTicker,
            calculateData,
            date,
            verbose);
        }
      }  
    }      

    latexReport << "\\end{multicols}" << std::endl;
    latexReport << "\\break"          << std::endl;
    latexReport << "\\newpage"        << std::endl;
  }

  latexReport << "\\end{document}" << std::endl;
  latexReport.close();

  if(verbose){
    if(!skip){
      std::cout  << tickerMetaData.primaryTicker << std::endl;
    }else{
      std::cout << " Skipping " << tickerMetaData.primaryTicker << std::endl;
    }       
  }

  return !skip;
};

//==============================================================================
int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string calculateDataFolder;
  std::string fundamentalFolder;
  std::string historicalFolder;
  std::string plotConfigurationFilePath;  
  std::string reportFolder;
  
  std::string singleFileToEvaluate;

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

    TCLAP::ValueArg<std::string> singleFileToEvaluateInput("i",
      "single_ticker_name", 
      "To evaluate a single ticker only, set the ticker name here.",
      false,"","string");
    cmd.add(singleFileToEvaluateInput);    

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

    singleFileToEvaluate      = singleFileToEvaluateInput.getValue();
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

      std::cout << "  Single file name to evaluate" << std::endl;
      std::cout << "    " << singleFileToEvaluate << std::endl;

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

  bool replaceNansWithMissingData = true;

  PlottingFunctions::PlotSettings plotSettings;
  std::vector< std::vector < std::string > > subplotMetricNames;
  readConfigurationFile(plotConfigurationFilePath,subplotMetricNames);

  std::vector< std::string > processedTickers;

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

    if(singleFileToEvaluate.length() > 0){
      fileName = singleFileToEvaluate;
    }    

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
      if(loadedFundData){
        loadedFundData = !fundamentalData.empty();
      }
      nlohmann::ordered_json historicalData;
      bool loadedHistData =JsonFunctions::loadJsonFile(ticker, 
                                historicalFolder, historicalData, verbose); 
      if(loadedHistData){
        loadedHistData = !historicalData.empty();
      }
                                
      nlohmann::ordered_json calculateData;
      bool loadedCalculateData =JsonFunctions::loadJsonFile(ticker, 
                                calculateDataFolder, calculateData, verbose);
      if(loadedCalculateData){
        loadedCalculateData = !calculateData.empty();
      }


      if(loadedFundData && loadedHistData && loadedCalculateData){


        TickerMetaData tickerMetaData;
        getTickerMetaData(fundamentalData, tickerMetaData);
        bool isTickerProcessed = isNameInList(tickerMetaData.primaryTicker, 
                                            processedTickers);  

        //
        // Generate and save the pdf
        //
        if(!isTickerProcessed && tickerMetaData.companyName.length() > 0){

          std::filesystem::path outputPlotFilePath = outputFolderPath;
          std::string plotFileName("fig_");
          plotFileName.append(tickerFolderName);
          plotFileName.append(".pdf");
          outputPlotFilePath.append(plotFileName);
        
          bool successPlotTickerData = 
            plotTickerData(
              tickerMetaData,
              fundamentalData,
              historicalData,
              calculateData,
              outputPlotFilePath.c_str(),
              subplotMetricNames,
              plotSettings,
              false);

          std::filesystem::path outputReportFilePath = outputFolderPath;
          std::string reportFileName(tickerFolderName);
          reportFileName.append(".tex");
          outputReportFilePath.append(reportFileName);

          bool successGenerateLaTeXReport = 
            generateLaTeXReport(
              tickerMetaData,
              fundamentalData,
              historicalData,
              calculateData,
              plotFileName.c_str(),
              outputReportFilePath.c_str(),              
              subplotMetricNames,
              replaceNansWithMissingData,
              false); 

          std::string wrapperFileName("report_");
          wrapperFileName.append(reportFileName);

          outputReportFilePath = outputFolderPath;
          outputReportFilePath.append(wrapperFileName);

          bool successGenerateLatexReportWrapper =
            generateLaTeXReportWrapper(outputReportFilePath.c_str(), 
                                       reportFileName, 
                                       tickerMetaData,
                                       false);

          processedTickers.push_back(tickerMetaData.primaryTicker);

          if(verbose){
            if(  !successPlotTickerData 
              || !successGenerateLaTeXReport  
              || !successGenerateLatexReportWrapper){
              std::cout << '\t' << "skipping" << std::endl;
              std::cout << '\t' << successPlotTickerData 
                        << '\t' << "plotting" << std::endl;
              std::cout << '\t' << successGenerateLaTeXReport 
                        << '\t' << "report generation" << std::endl;
              std::cout << '\t' << successGenerateLatexReportWrapper 
                        << '\t' << "report wrapper generation" << std::endl;

            }
          }

          ++validFileCount;     
        }
      }


    }

    if(singleFileToEvaluate.length() ){
      break;
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
