//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT

#ifndef PLOTTING_FUNCTIONS
#define PLOTTING_FUNCTIONS

#include <vector>

#include <iostream>
#include <sstream>

#include "JsonFunctions.h"


const double PointsPerInch    = 72;
const double InchesPerCentimeter  = 1.0/2.54;
const double Percentiles[5]     ={0.05, 0.25, 0.5, 0.75, 0.95};

class PlottingFunctions {

public:

  struct LineSettings{
    std::string colour;
    std::string name;
    double lineWidth;
  };

  struct BoxAndWhiskerSettings{
    double xOffsetFromStart;
    double xOffsetFromEnd;
    std::string boxWhiskerColour;
    std::string currentValueColour;
    BoxAndWhiskerSettings():
      xOffsetFromStart(std::nan("1")),
      xOffsetFromEnd(std::nan("1")),  
      boxWhiskerColour("blue"),
      currentValueColour("black"){};
  };


  struct AxisSettings{
    std::string xAxisName;
    std::string yAxisName;
    double xMin;
    double xMax;  
    double yMin;
    double yMax;
    bool isXMinFixed;
    bool isXMaxFixed;  
    bool isYMinFixed;
    bool isYMaxFixed;
    AxisSettings():
      xAxisName(""),
      yAxisName(""),
      xMin(std::nan("1")),
      xMax(std::nan("1")),
      yMin(std::nan("1")),
      yMax(std::nan("1")),
      isXMinFixed(false),
      isXMaxFixed(false),
      isYMinFixed(false),
      isYMaxFixed(false){};
  };

  struct SubplotSettings{
    size_t indexRow;
    size_t indexColumn;
  };

  struct PlotSettings{
    std::string fontName;
    double axisLabelFontSize;
    double axisTickFontSize;
    double legendFontSize;
    double titleFontSize;  
    double lineWidth;
    double axisLineWidth;
    double plotWidthInPoints;
    double plotHeightInPoints;
    double canvasWidth;
    double canvasHeight;
    double xticMinimumIncrement;
    bool logScale;
    PlotSettings():
      fontName("Times"),
      axisLabelFontSize(12.0),
      axisTickFontSize(12.0),
      legendFontSize(12.0),
      titleFontSize(14.0),
      lineWidth(1),
      axisLineWidth(1.0),
      plotWidthInPoints(8.0*InchesPerCentimeter*PointsPerInch),
      plotHeightInPoints(8.0*InchesPerCentimeter*PointsPerInch),
      canvasWidth(0.),
      canvasHeight(0.),
      xticMinimumIncrement(1.0),
      logScale(false){}  
  };  

  //==============================================================================
  static double convertCentimetersToPoints(double centimeters){
    return centimeters*InchesPerCentimeter*PointsPerInch;
  };

  //==============================================================================
  static double convertInchesToPoints(double inches){
    return inches*PointsPerInch;
  };

  //==============================================================================
  static void configurePlot(sciplot::Plot2D &plotUpd,
          std::string &xAxisLabel,
          std::string &yAxisLabel,
          const PlottingFunctions::PlotSettings &settings){

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

    if(settings.logScale){
      plotUpd.ytics().logscale(10.0);
    }

    plotUpd.size(settings.plotWidthInPoints,
           settings.plotHeightInPoints);

    };  
  //==========================================================================
  enum PercentileIndices{
  P05=0,
  P25,
  P50,
  P75,
  P95,
  NUM_PERCENTILES
  };

  //==========================================================================
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
    name(""){
    };
  };

  //==========================================================================
  static void getDataRange(   const std::vector< double > &data, 
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
    double yRange=ymax-ymin;
    if( yRange < minimumRange){
      ymax = 0.5*(ymax+ymin)+0.5*minimumRange;
      ymin = ymax - minimumRange;
    }

    dataRange.clear();
    dataRange.push_back(ymin);
    dataRange.push_back(ymax);
  }

  //==========================================================================
  // Note: These summary statistics are interpolated so that this method
  //     will give a sensible response with 1 data point or many.
  static bool extractSummaryStatistics(const std::vector< double > &data, 
                     SummaryStatistics &summary){

    bool validSummaryStatistics = true;                      

    if(data.size() > 0){
    std::vector<double> dataCopy;
    for(size_t i=0; i<data.size();++i){
      dataCopy.push_back(data[i]);
    }

    summary.current = std::nan("-1");

    std::sort(dataCopy.begin(),dataCopy.end());

    summary.min = dataCopy[0];
    summary.max = dataCopy[dataCopy.size()-1];

    if(dataCopy.size() > 1){
      for(size_t i = 0; i < PercentileIndices::NUM_PERCENTILES; ++i){
      double idx = Percentiles[i]*(dataCopy.size()-1);
      int indexA = std::floor(idx);
      int indexB = std::ceil(idx);
      double weightB = idx - static_cast<double>(indexA);
      double weightA = 1.0-weightB;
      double valueA = dataCopy[indexA];
      double valueB = dataCopy[indexB];
      double value = valueA*weightA + valueB*weightB;
      summary.percentiles.push_back(value);
      }
    }else{
      for(size_t i = 0; i < PercentileIndices::NUM_PERCENTILES; ++i){
        summary.percentiles.push_back(dataCopy[0]);
      }
      validSummaryStatistics=false;
    }
    }else{
    validSummaryStatistics=false;
    }
    return validSummaryStatistics;
  };

  //==========================================================================
  static void drawBoxAndWhisker(  sciplot::Plot2D &plotUpd,
                  double x,   
                  double xWidth,   
                  const SummaryStatistics &summary,
                  const char* boxColor,
                  const char* currentColor,
                  const int currentLineType,
                  const PlotSettings &settings,
                  bool verbose){
  sciplot::Vec xBox(5);
  sciplot::Vec yBox(5);

  xBox[0]= x-xWidth*0.5;
  xBox[1]= x+xWidth*0.5;
  xBox[2]= x+xWidth*0.5;
  xBox[3]= x-xWidth*0.5;
  xBox[4]= x-xWidth*0.5;

  yBox[0]= summary.percentiles[P25];
  yBox[1]= summary.percentiles[P25];
  yBox[2]= summary.percentiles[P75];
  yBox[3]= summary.percentiles[P75];
  yBox[4]= summary.percentiles[P25];

  plotUpd.drawCurveFilled(xBox,yBox)
      .fillColor(boxColor)
      .lineWidth(0.)
      .labelNone();

  sciplot::Vec xLine(2);
  sciplot::Vec yLine(2);

  xLine[0] = x;
  xLine[1] = x;
  yLine[0] = summary.percentiles[P75];
  yLine[1] = summary.max;

  plotUpd.drawCurve(xLine,yLine)
      .lineColor(boxColor)
      .lineWidth(settings.lineWidth)
      .dashType(1)
      .labelNone();

  xLine[0] = x;
  xLine[1] = x;
  yLine[0] = summary.percentiles[P75];
  yLine[1] = summary.percentiles[P95];

  plotUpd.drawCurve(xLine,yLine)
      .lineColor(boxColor)
      .lineWidth(settings.lineWidth*2)
      .labelNone();

  xLine[0] = x-xWidth*0.5;
  xLine[1] = x+xWidth*0.5;
  yLine[0] = summary.percentiles[P95];
  yLine[1] = summary.percentiles[P95];

  plotUpd.drawCurve(xLine,yLine)
      .lineColor(boxColor)
      .lineWidth(settings.lineWidth*2)
      .labelNone();        

  xLine[0] = x;
  xLine[1] = x;
  yLine[0] = summary.percentiles[P25];
  yLine[1] = summary.min;

  plotUpd.drawCurve(xLine,yLine)
      .lineColor(boxColor)
      .lineWidth(settings.lineWidth)
      .dashType(1)
      .labelNone();

  xLine[0] = x;
  xLine[1] = x;
  yLine[0] = summary.percentiles[P25];
  yLine[1] = summary.percentiles[P05];

  plotUpd.drawCurve(xLine,yLine)
      .lineColor(boxColor)
      .lineWidth(settings.lineWidth*2)
      .labelNone();

  xLine[0] = x-xWidth*0.5;
  xLine[1] = x+xWidth*0.5;
  yLine[0] = summary.percentiles[P05];
  yLine[1] = summary.percentiles[P05];

  plotUpd.drawCurve(xLine,yLine)
      .lineColor(boxColor)
      .lineWidth(settings.lineWidth*2)
      .labelNone();      

  xLine[0] = x-xWidth;
  xLine[1] = x+xWidth;
  yLine[0] = summary.percentiles[P50];
  yLine[1] = summary.percentiles[P50];

  plotUpd.drawCurve(xLine,yLine)
      .lineColor("white")
      .lineWidth(settings.lineWidth)
      .labelNone();

  xLine[0] = x-xWidth;
  xLine[1] = x+xWidth;
  yLine[0] = summary.current;
  yLine[1] = summary.current;

  plotUpd.drawCurve(xLine,yLine)
      .lineColor("gray100")
      .lineWidth(settings.lineWidth*2)
      .labelNone();

   
  xLine[0] = x-xWidth;
  xLine[1] = x+xWidth;
  yLine[0] = summary.current;
  yLine[1] = summary.current;

  plotUpd.drawCurve(xLine,yLine)
      .lineColor(currentColor)
      .lineWidth(settings.lineWidth)
      .lineType(currentLineType)      
      .labelNone();

  };

//==============================================================================
  static void updatePlot(
      const std::vector< double > &xV,
      const std::vector< double > &yV,
      const std::string &dataName,
      const PlottingFunctions::PlotSettings &plotSettings,    
      const LineSettings &lineSettings,
      AxisSettings &axisSettingsUpd,
      const BoxAndWhiskerSettings &boxAndWhiskerSettings,
      sciplot::Plot2D &plotMetricUpd,
      bool removeInvalidData,
      bool verbose)
  {

  
    
    bool timeSeriesValid = (xV.size()>0 && yV.size() > 0);

    if(timeSeriesValid || !removeInvalidData){
    

      sciplot::Vec xSci(xV.size());
      sciplot::Vec ySci(yV.size());

      if(!timeSeriesValid){
        xSci.resize(1);
        ySci.resize(1);
        xSci[0] = JsonFunctions::MISSING_VALUE;
        ySci[0] = JsonFunctions::MISSING_VALUE;
      }else{
        for(size_t i=0; i<xV.size();++i){
        xSci[i]= xV[i];
        ySci[i]= yV[i];
        }
      }


      PlottingFunctions::SummaryStatistics metricSummaryStatistics;
      metricSummaryStatistics.name=dataName;
      bool validSummaryStats = 
        PlottingFunctions::extractSummaryStatistics(yV,
                  metricSummaryStatistics);
      metricSummaryStatistics.current = yV[yV.size()-1];

      plotMetricUpd.drawCurve(xSci,ySci)
        .label(lineSettings.name)
        .lineColor(lineSettings.colour)
        .lineWidth(lineSettings.lineWidth);
      

      std::vector< double > xRange,yRange;
      PlottingFunctions::getDataRange(xV,xRange,1.0);
      PlottingFunctions::getDataRange(yV,yRange,
                std::numeric_limits< double >::lowest());
      if(yRange[0] > 0){
        yRange[0] = 0;
      }
      double ySpan = yRange[1]-yRange[0];
      if( ySpan <= 0){
        yRange[1] = 1.0;
      }
      //Add some blank space to the top of the plot
      yRange[1] = yRange[1] + 0.5*(yRange[1]-yRange[0]);
      int currentLineType=1;

      if(!std::isnan(axisSettingsUpd.xMin)){
        //if(xRange[0] > axisSettingsUpd.xMin){
          xRange[0]=axisSettingsUpd.xMin;
        //}
      }

      if(!std::isnan(axisSettingsUpd.xMax)){
        //if(xRange[1]<axisSettingsUpd.xMax){
          xRange[1]=axisSettingsUpd.xMax;
        //}
      }

      if(!std::isnan(axisSettingsUpd.yMin)){
        //if(yRange[0]>axisSettingsUpd.yMin){
          yRange[0]=axisSettingsUpd.yMin;
        //}
      }

      if(!std::isnan(axisSettingsUpd.yMax)){
        //if(yRange[1]<axisSettingsUpd.yMax){
          yRange[1]=axisSettingsUpd.yMax;
        //}
      }

      plotMetricUpd.legend().atTopLeft();   

      if(validSummaryStats){
        double xPos = xRange[1]+1;
        if(!std::isnan(boxAndWhiskerSettings.xOffsetFromStart)){
        xPos = xRange[0]+boxAndWhiskerSettings.xOffsetFromStart;
        }
        if(!std::isnan(boxAndWhiskerSettings.xOffsetFromEnd)){
        xPos = xRange[1]+boxAndWhiskerSettings.xOffsetFromEnd;      
        }

        PlottingFunctions::drawBoxAndWhisker(
          plotMetricUpd,
          xPos,
          0.5,
          metricSummaryStatistics,
          boxAndWhiskerSettings.boxWhiskerColour.c_str(),
          boxAndWhiskerSettings.currentValueColour.c_str(),
          currentLineType,
          plotSettings,
          verbose);
      }

      if(!std::isnan(boxAndWhiskerSettings.xOffsetFromStart)){
        xRange[0] += (boxAndWhiskerSettings.xOffsetFromStart-1);
      }
      if(!std::isnan(boxAndWhiskerSettings.xOffsetFromEnd)){
        xRange[1] += (boxAndWhiskerSettings.xOffsetFromEnd+1);      
      }

      //Update the axis so that the axis grow to include all data
      axisSettingsUpd.xMin = std::min(xRange[0],axisSettingsUpd.xMin);
      axisSettingsUpd.xMax = std::max(xRange[1],axisSettingsUpd.xMax);
      axisSettingsUpd.yMin = std::min(yRange[0],axisSettingsUpd.yMin);
      axisSettingsUpd.yMax = std::max(yRange[1],axisSettingsUpd.yMax);


      plotMetricUpd.xrange(
        static_cast<sciplot::StringOrDouble>(xRange[0]),
        static_cast<sciplot::StringOrDouble>(xRange[1]));        

      plotMetricUpd.yrange(
        static_cast<sciplot::StringOrDouble>(yRange[0]),
        static_cast<sciplot::StringOrDouble>(yRange[1]));

      if((xRange[1]-xRange[0])<5.0){
        plotMetricUpd.xtics().increment(plotSettings.xticMinimumIncrement);
      }

      if((xRange[1]-xRange[0]) > 10){
        double xSpan = (xRange[1]-xRange[0]);
        double increment = 1;

        while( xSpan/increment > 10 ){
        if(std::abs(increment-1) < std::numeric_limits<double>::epsilon()){
          increment = 5.0;
        }else{
          increment += 5.0;
        }
        }

        plotMetricUpd.xtics().increment(increment);

      }


      PlottingFunctions::configurePlot( plotMetricUpd,
                        axisSettingsUpd.xAxisName,
                        axisSettingsUpd.yAxisName,
                        plotSettings);

    }


  };

//==============================================================================
  static void writePlot(
    const std::vector< std::vector < sciplot::Plot2D >> &matrixOfPlots,
    const PlottingFunctions::PlotSettings &plotSettings,  
    const std::string &titleStr,  
    const std::string &outputPlotPath)
  {

    size_t nrows = matrixOfPlots.size();
    size_t ncols = matrixOfPlots[0].size();

    std::vector< std::vector < sciplot::PlotVariant > > arrayOfPlotVariants;  

    for(size_t indexRow=0; indexRow < nrows; ++indexRow){
      std::vector< sciplot::PlotVariant > rowOfPlotVariants;  
      for(size_t indexCol=0; indexCol < ncols; ++indexCol){
      rowOfPlotVariants.push_back(matrixOfPlots[indexRow][indexCol]);
      }
      arrayOfPlotVariants.push_back(rowOfPlotVariants);
    }
    
    sciplot::Figure figTicker(arrayOfPlotVariants);

    figTicker.title(titleStr);

    sciplot::Canvas canvas = {{figTicker}};

    size_t canvasWidth  = 
      static_cast<size_t>(
      plotSettings.plotWidthInPoints*static_cast<double>(ncols));
    size_t canvasHeight = 
      static_cast<size_t>(
      plotSettings.plotHeightInPoints*static_cast<double>(nrows));

    canvas.size(canvasWidth, canvasHeight) ;

    // Save the figure to a PDF file
    canvas.save(outputPlotPath);

  };

};


#endif
