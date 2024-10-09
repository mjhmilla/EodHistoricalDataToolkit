#ifndef PLOTTING_FUNCTIONS
#define PLOTTING_FUNCTIONS

#include <vector>

#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

#include "JsonFunctions.h"
#include "UtilityFunctions.h"


const double PointsPerInch        = 72;
const double InchesPerCentimeter  = 1.0/2.54;
const double Percentiles[5]       ={0.05, 0.25, 0.5, 0.75, 0.95};

class PlottingFunctions {

public:

    //==========================================================================
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
    static void configurePlot(sciplot::Plot2D &plotUpd,
                    const std::string &xAxisLabel,
                    const std::string &yAxisLabel,
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

        plotUpd.size(settings.plotWidth,settings.plotHeight);

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
        name(""){};
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
        if(ymax-ymin < minimumRange){
            ymax = 0.5*(ymax+ymin)+0.5*minimumRange;
            ymin = ymax - minimumRange;
        }

        dataRange.clear();
        dataRange.push_back(ymin);
        dataRange.push_back(ymax);
    }

    //==========================================================================
    static void extractSummaryStatistics(sciplot::Vec &data, 
                                         SummaryStatistics &summary){

      if(data.size() > 0){
        std::vector<double> dataDbl;
        for(size_t i=0; i<data.size(); ++i){
            dataDbl.push_back(data[i]);
        }
        summary.current = dataDbl[dataDbl.size()-1];

        std::sort(dataDbl.begin(),dataDbl.end());

        summary.min = dataDbl[0];
        summary.max = dataDbl[dataDbl.size()-1];


        if(dataDbl.size() > 1){
            for(size_t i = 0; i < PercentileIndices::NUM_PERCENTILES; ++i){
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
            for(size_t i = 0; i < PercentileIndices::NUM_PERCENTILES; ++i){
            summary.percentiles.push_back(dataDbl[0]);
            }
        }
      }
    };

    //==========================================================================
    static void drawBoxAndWhisker(  sciplot::Plot2D &plotUpd,
                                    double x,   
                                    double xWidth,   
                                    const SummaryStatistics &summary,
                                    const char* lineColor,
                                    const char* currentColor,
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

    plotUpd.drawCurve(xBox,yBox)
            .lineColor(lineColor)
            .lineWidth(settings.lineWidth*2)
            .labelNone();

    sciplot::Vec xLine(2);
    sciplot::Vec yLine(2);

    xLine[0] = x;
    xLine[1] = x;
    yLine[0] = summary.percentiles[P75];
    yLine[1] = summary.max;

    plotUpd.drawCurve(xLine,yLine)
            .lineColor(lineColor)
            .lineWidth(settings.lineWidth)
            .dashType(1)
            .labelNone();

    xLine[0] = x;
    xLine[1] = x;
    yLine[0] = summary.percentiles[P75];
    yLine[1] = summary.percentiles[P95];

    plotUpd.drawCurve(xLine,yLine)
            .lineColor(lineColor)
            .lineWidth(settings.lineWidth*2)
            .labelNone();

    xLine[0] = x-xWidth*0.5;
    xLine[1] = x+xWidth*0.5;
    yLine[0] = summary.percentiles[P95];
    yLine[1] = summary.percentiles[P95];

    plotUpd.drawCurve(xLine,yLine)
            .lineColor(lineColor)
            .lineWidth(settings.lineWidth*2)
            .labelNone();              

    xLine[0] = x;
    xLine[1] = x;
    yLine[0] = summary.percentiles[P25];
    yLine[1] = summary.min;

    plotUpd.drawCurve(xLine,yLine)
            .lineColor(lineColor)
            .lineWidth(settings.lineWidth)
            .dashType(1)
            .labelNone();

    xLine[0] = x;
    xLine[1] = x;
    yLine[0] = summary.percentiles[P25];
    yLine[1] = summary.percentiles[P05];

    plotUpd.drawCurve(xLine,yLine)
            .lineColor(lineColor)
            .lineWidth(settings.lineWidth*2)
            .labelNone();

    xLine[0] = x-xWidth*0.5;
    xLine[1] = x+xWidth*0.5;
    yLine[0] = summary.percentiles[P05];
    yLine[1] = summary.percentiles[P05];

    plotUpd.drawCurve(xLine,yLine)
            .lineColor(lineColor)
            .lineWidth(settings.lineWidth*2)
            .labelNone();            

    xLine[0] = x-xWidth*0.5;
    xLine[1] = x+xWidth*0.5;
    yLine[0] = summary.percentiles[P50];
    yLine[1] = summary.percentiles[P50];

    plotUpd.drawCurve(xLine,yLine)
            .lineColor(lineColor)
            .lineWidth(settings.lineWidth)
            .labelNone();

    xLine[0] = x-xWidth*0.5;
    xLine[1] = x+xWidth*0.5;
    yLine[0] = summary.current;
    yLine[1] = summary.current;

    plotUpd.drawCurve(xLine,yLine)
            .lineColor("gray100")
            .lineWidth(settings.lineWidth*3)
            .labelNone();

    xLine[0] = x-xWidth*0.5;
    xLine[1] = x+xWidth*0.5;
    yLine[0] = summary.current;
    yLine[1] = summary.current;

    plotUpd.drawCurve(xLine,yLine)
            .lineColor(currentColor)
            .lineWidth(settings.lineWidth)
            .labelNone();

    };
};


#endif
