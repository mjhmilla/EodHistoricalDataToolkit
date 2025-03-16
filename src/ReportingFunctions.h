//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT

#ifndef REPORTING_FUNCTIONS
#define REPORTING_FUNCTIONS

#include <vector>

#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

#include "JsonFunctions.h"
#include "DateFunctions.h"

class ReportingFunctions {

public:


//==============================================================================
static void sanitizeFolderName(std::string &folderName, 
                               bool stripExtension=false){

  if(stripExtension){
    size_t idx = folderName.find_last_of('.');
    if(idx != folderName.npos){
      folderName = folderName.substr(0,idx);
    }
  }

  std::string charactersToDelete("");  
  charactersToDelete.append("\'");
  charactersToDelete.append(" ");

  deleteCharacters(folderName,charactersToDelete);

  std::string charactersToEscape("");
  charactersToEscape.append("&");
  charactersToEscape.append("$");
  charactersToEscape.append("#");
  charactersToEscape.append(".");

  std::string replacementCharacter("_");
  replaceSpecialCharacters(folderName,
                           charactersToEscape,
                           replacementCharacter);
};
//==============================================================================
static void sanitizeLabelForLaTeX(std::string &stringForLatex,
                                  bool stripExtension=false){

  if(stripExtension){
    size_t idx = stringForLatex.find_last_of('.');
    if(idx != stringForLatex.npos){
      stringForLatex = stringForLatex.substr(0,idx);
    }
  }

  std::string charactersToDelete("");  
  charactersToDelete.append("\'");
  charactersToDelete.append(" ");
  charactersToDelete.append("&");
  charactersToDelete.append("$");
  charactersToDelete.append("#");

  deleteCharacters(stringForLatex,charactersToDelete);

  std::string charactersToEscape("");
  charactersToEscape.append(".");

  std::string replacementCharacter("-");
  replaceSpecialCharacters(stringForLatex,
                           charactersToEscape,
                           replacementCharacter);
};
//==============================================================================

static void sanitizeStringForLaTeX(std::string &stringForLatex,
                                   bool stripExtension=false){

  if(stripExtension){
    size_t idx = stringForLatex.find_last_of('.');
    if(idx != stringForLatex.npos){
      stringForLatex = stringForLatex.substr(0,idx);
    }
  }                                    

  std::string charactersToDelete("");  
  charactersToDelete.append("\'");

  ReportingFunctions::deleteCharacters(stringForLatex,charactersToDelete);

  std::string charactersToEscape("");
  charactersToEscape.append("&");
  charactersToEscape.append("$");
  charactersToEscape.append("#");
  charactersToEscape.append("_");

  ReportingFunctions::escapeSpecialCharacters(stringForLatex, charactersToEscape);

};

//==============================================================================
static void convertCamelCaseToSpacedText(std::string &labelUpd){
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

};
//==============================================================================
static void replaceSpecialCharacters(std::string &textUpd, 
                            const std::string &charactersToReplace,
                            const std::string &replacementString){

    for(size_t i=0; i<charactersToReplace.length();++i){
      char charToReplace = charactersToReplace[i];
      size_t idx = textUpd.find(charToReplace,0);
      while(idx != std::string::npos){
        if(idx != std::string::npos){
          textUpd.replace(idx, replacementString.length(),
                               replacementString);
        }
        idx = textUpd.find(charToReplace,idx+1);
      }
    }
};
//==============================================================================
static void escapeSpecialCharacters(std::string &textUpd, 
                            const std::string &charactersToEscape){

    for(size_t i=0; i<charactersToEscape.length();++i){
      char charToEscape = charactersToEscape[i];
      size_t idx = textUpd.find(charToEscape,0);
      while(idx != std::string::npos){
        if(idx != std::string::npos){
          textUpd.insert(idx,"\\");
        }
        idx = textUpd.find(charToEscape,idx+2);
      }
    }
};
//==============================================================================
static void deleteCharacters(std::string &textUpd, 
                             const std::string &charactersToDelete){

    for(size_t i=0; i< charactersToDelete.length();++i){
      char delChar = charactersToDelete[i];
      size_t idx = textUpd.find(delChar,0);
      while(idx != std::string::npos){
        if(idx != std::string::npos){
          textUpd.erase(idx,1);
        }
        idx = textUpd.find(delChar,idx);
      }
    }
};


//==============================================================================
static std::string formatJsonEntry( double entry, 
                                    int roundToDigits = 3,
                                    bool addNegativeHighlight=false){


  std::stringstream ss;
  if(!addNegativeHighlight){
    if(JsonFunctions::isJsonFloatValid(entry)){
      if(roundToDigits > 1){
        ss << std::setprecision(roundToDigits) << entry;
      }else{
        ss << entry;          
      }
    }else{
      if(roundToDigits > 1){       
        ss << "\\cellcolor{RedOrange}" << std::setprecision(roundToDigits)  
           << std::round(entry);
      }else{
        ss << "\\cellcolor{RedOrange}" << std::round(entry);
      }
    }
  }else{
    if(entry < 0.){
      if(roundToDigits > 1){
        ss << "\\cellcolor{BurntOrange}" 
           << std::setprecision(roundToDigits) << entry;
      }else{
        ss << "\\cellcolor{BurntOrange}" << entry;
      }
    }else{    
      if(roundToDigits > 1){
        ss << std::setprecision(roundToDigits) << entry;
      }else{
        ss << entry;
      }
    }     
  }
  
  return ss.str();
}
//==============================================================================
static void appendCashFlowConversionTable(
                std::ofstream &latexReport, 
                const std::string &primaryTicker, 
                const nlohmann::ordered_json &calculateData,
                const std::string &date,
                bool verbose)
{
    if(calculateData[date].contains("cashFlowConversionRatio")){
        latexReport << "\\begin{tabular}{l l}" << std::endl;
        latexReport << "\\hline \\multicolumn{2}{c}{Cash flow conversion ratio} \\\\" 
                    << std::endl;
        latexReport << "\\hline " << std::endl;                    
        latexReport << "$A$. Total cash from Op. Activities & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["cashFlowConversionRatio_freeCashFlow_totalCashFromOperatingActivities"],true))
                    << " \\\\" << std::endl;
        latexReport << "$B$. Interest Expense & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["cashFlowConversionRatio_freeCashFlow_interestExpense"],true))
                    << " \\\\" << std::endl;
        latexReport << "$C$. Tax Rate & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["cashFlowConversionRatio_freeCashFlow_taxRate"],true))
                    << " \\\\" << std::endl;
        latexReport << "$D$. Tax shield on interest exp. & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["cashFlowConversionRatio_freeCashFlow_taxShieldOnInterestExpense"],true))
                    << " \\\\" << std::endl;
        latexReport << "\\multicolumn{2}{c}{ $D = B\\,C$} \\\\" 
                    << std::endl;  
        latexReport << "$E$. Capital Expenditures & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["cashFlowConversionRatio_freeCashFlow_capitalExpenditures"],true))
                    << " \\\\" << std::endl;            
        latexReport << "$F$. Free cash flow (calc) & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["cashFlowConversionRatio_freeCashFlow_freeCashFlow"],true))
                    << " \\\\" << std::endl;            
        latexReport << "\\multicolumn{2}{c}{ $F = A+B-D-E$} \\\\" 
                    << std::endl;           
        latexReport << "$G$. Free cash flow (EOD) & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["cashFlowConversionRatio_freeCashFlow_freeCashFlowEOD"],true))
                    << " \\\\" << std::endl;  
        latexReport << "$H$. Net income & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["cashFlowConversionRatio_netIncome"],true))
                    << " \\\\" << std::endl;            
        latexReport << "$I$. Cash flow conversion ratio & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["cashFlowConversionRatio"],true))
                    << " \\\\" << std::endl;
        latexReport << "\\multicolumn{2}{c}{ $I = F/H$} \\\\" 
                    << std::endl;           
        latexReport << "\\end{tabular}" << std::endl 
                    << "\\bigskip" << std::endl<< std::endl;  
    }
};
//==============================================================================
static void appendOperatingMarginTable(
                std::ofstream &latexReport, 
                const std::string &primaryTicker, 
                const nlohmann::ordered_json &calculateData,
                const std::string &date,
                bool verbose)
{
    if(calculateData[date].contains("operatingMargin")){
        latexReport << "\\begin{tabular}{l l}" << std::endl;
        latexReport << "\\hline \\multicolumn{2}{c}{Operating Margin} \\\\" 
                    << std::endl;
        latexReport << "\\hline " << std::endl;                    
        latexReport << "$A$. Operating Income & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["operatingMargin_operatingIncome"],true))
                    << " \\\\" << std::endl;
        latexReport << "$B$. Total Revenue & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["operatingMargin_totalRevenue"],true))
                    << " \\\\" << std::endl;
        latexReport << "$C$. Operating Margin & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["operatingMargin"],true))
                    << " \\\\" << std::endl;
        latexReport << "\\multicolumn{2}{c}{ $C = A/B$} \\\\" 
                    << std::endl;  
        latexReport << "\\end{tabular}" << std::endl 
                    << "\\bigskip" << std::endl<< std::endl;  
    }
};
//==============================================================================
static void appendResidualCashflowToEnterpriseValueTable(
                std::ofstream &latexReport, 
                const std::string &primaryTicker, 
                const nlohmann::ordered_json &calculateData,
                const std::string &date,
                bool verbose)
{

    if(calculateData[date].contains("residualCashFlow")){
        latexReport << "\\begin{tabular}{l l}" << std::endl;
        latexReport << "\\hline \\multicolumn{2}{c}{Residual Cash Flow} \\\\" 
                    << std::endl;
        latexReport << "\\hline " << std::endl;                    
        latexReport << "$A$. Total cash from Operating Activities & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["residualCashFlow_totalCashFromOperatingActivities"],true))
                    << " \\\\" << std::endl;
        latexReport << "$B$. Research \\& Development & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["residualCashFlow_researchDevelopment"],true))
                    << " \\\\" << std::endl;
        latexReport << "$C$. Mean Capital Expenditure & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["residualCashFlow_capitalExpenditureMean"],true))
                    << " \\\\" << std::endl;
        latexReport << "$D$. Total Stockholder Equity & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["residualCashFlow_totalStockholderEquity"],true))
                    << " \\\\" << std::endl; 
        latexReport << "$E$. Cost Of Equity (\\%) & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["residualCashFlow_costOfEquityAsAPercentage"],true),
                        3,true)
                    << " \\\\" << std::endl;
        latexReport << "$F$. Cost Of Equity & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["residualCashFlow_costOfEquity"],true))
                    << " \\\\" << std::endl;
        latexReport << "$G$. Residual Cash Flow & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["residualCashFlow"],true))
                    << " \\\\" << std::endl;                 
        latexReport << "\\multicolumn{2}{c}{ $G = A+B-C-F$} \\\\" 
                    << std::endl;  
        latexReport << "\\end{tabular}" << std::endl 
                    << "\\bigskip" << std::endl<< std::endl;  
    }else{
        latexReport << "\\begin{tabular}{l l}" << std::endl;
        latexReport << "\\hline \\multicolumn{2}{c}{Residual Cash Flow to Enterprise Value} \\\\" 
                    << std::endl;
        latexReport << "\\multicolumn{2}{c}{Insufficient trailing past periods}"                    
                    << std::endl;
        latexReport << "\\end{tabular}" << std::endl 
                    << "\\bigskip" << std::endl<< std::endl;                      
    }

    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Enterprise Value} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;                    
    latexReport << "$A$. Market capitalization & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["enterpriseValue_marketCapitalization"],true))
                << " \\\\" << std::endl;  
    latexReport << "$B$. Debt book value & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["enterpriseValue_debtBookValue"],true))
                << " \\\\" << std::endl;     
    latexReport << "\\qquad Short-long term debt entry& "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["enterpriseValue_shortLongTermDebtTotal"],true))
                << " \\\\" << std::endl; 
    latexReport << "\\qquad Long term debt entry & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["enterpriseValue_longTermDebt"],true))
                << " \\\\" << std::endl; 
    latexReport << "$C$. Cash \\& Equivalents & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["enterpriseValue_cashAndEquivalentsEntry"],true))
                << " \\\\" << std::endl;   
    latexReport << "\\qquad Cash \\& Equivalents entry & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["enterpriseValue_cashAndEquivalents"],true))
                << " \\\\" << std::endl;   
    latexReport << "\\qquad Cash entry & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["enterpriseValue_cash"],true))
                << " \\\\" << std::endl;   
    latexReport << "$D$. Enterprise Value & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["enterpriseValue"],true))
                << " \\\\" << std::endl;                 
    latexReport << "\\multicolumn{2}{c}{ $D = A+B-C$} \\\\" 
                << std::endl;  
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;  

    if(calculateData[date].contains("residualCashFlow")){
        latexReport << "\\begin{tabular}{l l}" << std::endl;
        latexReport << "\\hline \\multicolumn{2}{c}{Residual Cash Flow (rFCF) To Enterprise value (EV)} \\\\" 
                    << std::endl;
        latexReport << "\\hline " << std::endl;
        latexReport << "$A$. Residual Cash Flow & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["residualCashFlow"],true))
                    << " \\\\" << std::endl;  
        latexReport << "$B$. Enterprise Value & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["enterpriseValue"],true))
                    << " \\\\" << std::endl;     
        latexReport << "$C$. rFCF / EV & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date]["residualCashFlowToEnterpriseValue"],true))
                    << " \\\\" << std::endl;     
        latexReport << "\\multicolumn{2}{c}{ $C = A/B$} \\\\" 
                    << std::endl;  
        latexReport << "\\end{tabular}" << std::endl 
                    << "\\bigskip" << std::endl<< std::endl;                      
    }else{
        latexReport << "\\begin{tabular}{l l}" << std::endl;
        latexReport << "\\hline \\multicolumn{2}{c}{Residual Cash Flow // Enterprise value} \\\\" 
                    << std::endl;
        latexReport << "\\hline " << std::endl;                       
        latexReport << "\\multicolumn{2}{c}{Insufficient trailing past periods}"                    
                    << std::endl;
        latexReport << "\\end{tabular}" << std::endl 
                    << "\\bigskip" << std::endl<< std::endl;                      

    }                
};

//==============================================================================
static void appendExcessReturnOnInvestedFinancialCapitalTable(
                std::ofstream &latexReport, 
                const std::string &primaryTicker, 
                const nlohmann::ordered_json &calculateData,
                const std::string &date,
                bool verbose)
{


    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Excess return on invested capital (Financial)} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;                    
    latexReport << "$A$. Debt book value & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital_debtBookValue"],true))
                << " \\\\" << std::endl; 
    latexReport << "\\qquad short-long term debt & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital_shortLongTermDebt"],true))
                << " \\\\" << std::endl; 
    latexReport << "\\qquad long term debt & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital_longTermDebt"],true))
                << " \\\\" << std::endl; 
    latexReport << "$B$. Other long term liabilities & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital_otherLongTermLiabilities"],true),
                    3,true)
                << " \\\\" << std::endl;
    latexReport << "$C$. Total stock holder equity & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital_totalStockholderEquity"],true),
                    3,true)
                << " \\\\" << std::endl;
    latexReport << "$D$. Cash & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital_cash"],true))
                << " \\\\" << std::endl;
    latexReport << "$E$. Operating income & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital_operatingIncome"],true))
                << " \\\\" << std::endl;
    latexReport << "$F$. Tax rate & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital_taxRate"],true))
                << " \\\\" << std::endl;
    latexReport << "$G$. After tax operating income & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital_afterTaxOperatingIncome"],true))
                << " \\\\" << std::endl;
    latexReport << "\\multicolumn{2}{c}{ $G = E\\,(1.0-F)$} \\\\" 
                << std::endl;                       
    latexReport << "$I$. Return on invested capital & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital"],true))
                << " \\\\" << std::endl;                 
    latexReport << "\\multicolumn{2}{c}{ $I = G/(A+B+C-D)$} \\\\" 
                << std::endl;  
    latexReport << "$H$. Cost of capital (mature) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["costOfCapitalMature"],true))
                << " \\\\" << std::endl;                 
    latexReport << "$I$. Excess return on invested capital & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapitalLessCostOfCapital"],true))
                << " \\\\" << std::endl;                 
    latexReport << "\\multicolumn{2}{c}{ $I = G-H$} \\\\" 
                << std::endl;  
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;  


};

//==============================================================================
static void appendExcessReturnOnInvestedOperationsCapitalTable(
                std::ofstream &latexReport, 
                const std::string &primaryTicker, 
                const nlohmann::ordered_json &calculateData,
                const std::string &date,
                bool verbose)
{

    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Excess return on invested capital (Operations)} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;                    
    latexReport << "$A$. Net working capital & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedOperatingCapital_netWorkingCapital"],true))
                << " \\\\" << std::endl; 
    latexReport << "$B$. Net property, plant \\& equipment & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedOperatingCapital_propertyPlantAndEquipmentNet"],true))
                << " \\\\" << std::endl;
    latexReport << "$C$. Intangible assets & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedOperatingCapital_intangibleAssets"],true))
                << " \\\\" << std::endl;
    latexReport << "$D$. Goodwill & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedOperatingCapital_goodWill"],true))
                << " \\\\" << std::endl;
    latexReport << "$E$. Other assets & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedOperatingCapital_otherAssets"],true))
                << " \\\\" << std::endl;                
    latexReport << "$F$. Operating income & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedOperatingCapital_operatingIncome"],true))
                << " \\\\" << std::endl;
    latexReport << "$G$. Tax rate & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedOperatingCapital_taxRate"],true))
                << " \\\\" << std::endl;
    latexReport << "$H$. After tax operating income & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedOperatingCapital_afterTaxOperatingIncome"],true))
                << " \\\\" << std::endl;
    latexReport << "\\multicolumn{2}{c}{ $H = F\\,(1.0-G)$} \\\\" 
                << std::endl;                       
    latexReport << "$I$. Return on invested capital & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedOperatingCapital"],true))
                << " \\\\" << std::endl;                 
    latexReport << "\\multicolumn{2}{c}{ $I = H/(A+B+C+D+E)$} \\\\" 
                << std::endl;  
    latexReport << "$J$. Cost of capital (mature) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["costOfCapitalMature"],true))
                << " \\\\" << std::endl;                 
    latexReport << "$K$. Excess return on invested capital & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedOperatingCapitalLessCostOfCapital"],true))
                << " \\\\" << std::endl;                 
    latexReport << "\\multicolumn{2}{c}{ $K = I-J$} \\\\" 
                << std::endl;                  
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;  

};

//==============================================================================
static void appendRetentionRatioTable(std::ofstream &latexReport, 
                           const std::string &primaryTicker, 
                           const nlohmann::ordered_json &calculateData,
                           const std::string &date,
                           bool verbose){

    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Retention ratio} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;                
    latexReport << "$A_8$. Net income & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["retentionRatio_netIncome"],true))
                << " \\\\" << std::endl;
    latexReport << "$B_8$. Dividends paid & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["retentionRatio_dividendsPaid"],true))
                << " \\\\" << std::endl;
    latexReport << "$C_8$. Retention ratio & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["retentionRatio"],true))
                << " \\\\" << std::endl;                
    latexReport << "\\multicolumn{2}{c}{ $C_8 = (A_8-B_8)/A_8$} \\\\" 
            << std::endl;   
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;         
};

//==============================================================================
static void appendReturnOnEquityTable(std::ofstream &latexReport, 
                           const std::string &primaryTicker, 
                           const nlohmann::ordered_json &calculateData,
                           const std::string &date,
                           bool verbose){

    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Return on equity} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;                
    latexReport << "$A_9$. Net income & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnEquity_netIncome"],true))
                << " \\\\" << std::endl;
    latexReport << "$B_9$. Total stock holder equity & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnEquity_totalStockholderEquity"],true),true)
                << " \\\\" << std::endl;
    latexReport << "$C_9$. Return on equity & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnEquity"],true))
                << " \\\\" << std::endl;                
    latexReport << "\\multicolumn{2}{c}{ $C_9 = A_9/B_9$} \\\\" 
            << std::endl;   
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl; 
};

//==============================================================================
static void appendNetIncomeGrowthTable(std::ofstream &latexReport, 
                           const std::string &primaryTicker, 
                           const nlohmann::ordered_json &calculateData,
                           const std::string &date,
                           bool verbose){
    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Net income growth} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;                
    latexReport << "$A_{10}$. Retention Ratio & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["retentionRatio"],true))
                << " \\\\" << std::endl;                
    latexReport << "$B_{10}$. Return on equity & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnEquity"],true))
                << " \\\\" << std::endl;                
    latexReport << "$C_{10}$. Net income growth & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["netIncomeGrowth"],true))
                << " \\\\" << std::endl;                
    latexReport << "\\multicolumn{2}{c}{ $C_{10} = C_8\\,C_9$} \\\\" 
            << std::endl;  
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;     
};

//==============================================================================
static void appendEmpiricalGrowthTable(std::ofstream &latexReport, 
                           const std::string &primaryTicker, 
                           const nlohmann::ordered_json &calculateData,
                           const std::string &date,
                           const std::string &name,
                           const std::string &title,
                           bool verbose){

    if(calculateData[date].contains(name+"AfterTaxOperatingIncomeGrowth")){
      //latexReport << "\\bigskip" << std::endl<< std::endl;  
      latexReport << "\\begin{tabular}{l l}" << std::endl;
      latexReport << " & \\\\" << std::endl;      
      latexReport << "\\hline \\multicolumn{2}{c}{"+title+"} \\\\" 
                  << std::endl;
      latexReport << "\\hline " << std::endl;                
      latexReport << "$A$. Lsq. After-tax op. income growth & "
                  << formatJsonEntry(JsonFunctions::getJsonFloat(
                      calculateData[date][name+"AfterTaxOperatingIncomeGrowth"],true))
                  << " \\\\" << std::endl;
      latexReport << "$B$. Lsq. After-tax op. income R2 & "
                  << formatJsonEntry(JsonFunctions::getJsonFloat(
                      calculateData[date][name+"ModelR2"],true))
                  << " \\\\" << std::endl;
      latexReport << "$C$. Years of data used in fit & "
                  << formatJsonEntry(JsonFunctions::getJsonFloat(
                      calculateData[date][name+"DataDuration"],true))
                  << " \\\\" << std::endl;
      latexReport << "$D$. Age of estimate & "
                  << formatJsonEntry(JsonFunctions::getJsonFloat(
                      calculateData[date][name+"ModelDateError"],true))
                  << " \\\\" << std::endl;
      latexReport << "$E$. Negative outliers set to 1.0  & "
                  << formatJsonEntry(JsonFunctions::getJsonFloat(
                      calculateData[date][name+"OutlierCount"],true))
                  << " \\\\" << std::endl;                  

      latexReport << "$F$. Model type & "
                  << formatJsonEntry(JsonFunctions::getJsonFloat(
                      calculateData[date][name+"ModelType"],true))
                  << " \\\\" << std::endl;  
      int type = 
          static_cast<int>(
              JsonFunctions::getJsonFloat(calculateData[date][name+"ModelType"]));
      switch(type){
        case 0:
        {
          latexReport << "\\multicolumn{2}{c}{ $y = f(1+g)^n$ (LSQ)} \\\\" 
              << std::endl;      
        }; 
        break;
        case 1:
        {
          latexReport << "\\multicolumn{2}{c}{ $y = A x + b$ (LSQ) } \\\\" 
              << std::endl;      
          latexReport << "\\multicolumn{2}{c}{ $y_0 = A x_0$} \\\\" 
              << std::endl;      
          latexReport << "\\multicolumn{2}{c}{ $y_1 = A x_1$} \\\\" 
              << std::endl;      
          latexReport << "\\multicolumn{2}{c}{ $g = \\exp(\\log(y_1/y_0)/N-1) } \\\\" 
              << std::endl;      
        }; 
        break;
        default:{
          std::cout << "Error: unrecognized empiricalModelType. Must be 0 or 1."
                    << std::endl;
          std::abort();            
        };
      };

                

      latexReport << "$G$. Avg. Reinvestment Rate & "
                  << formatJsonEntry(JsonFunctions::getJsonFloat(
                      calculateData[date][name+"ReinvestmentRateMean"],true))
                  << " \\\\" << std::endl;
      latexReport << "$H$. Reinvestment Rate S.D. & "
                  << formatJsonEntry(JsonFunctions::getJsonFloat(
                      calculateData[date][name+"ReinvestmentRateStandardDeviation"],true))
                  << " \\\\" << std::endl;

      latexReport << "$I$. Return on invested capital & "
                  << formatJsonEntry(JsonFunctions::getJsonFloat(
                      calculateData[date][name+"ReturnOnInvestedCapital"],true))
                  << " \\\\" << std::endl;                              
      latexReport << "\\multicolumn{2}{c}{ $I = A/G$} \\\\" 
              << std::endl;  
      latexReport << "$J$. Excess return on invested capital & "
                  << formatJsonEntry(JsonFunctions::getJsonFloat(
                      calculateData[date][name+"ReturnOnInvestedCapitalLessCostOfCapital"],true))
                  << " \\\\" << std::endl;                              

      latexReport << "\\end{tabular}" << std::endl;  
    }
                                 
};
//==============================================================================
// The 'ForValuation' ending means these tables cross reference other
// tables, and so, each table has a specific number
static void appendCostOfCapitalTableForValuation( std::ofstream &latexReport, 
                                      const std::string &primaryTicker, 
                                      const nlohmann::ordered_json &countryData,
                                      const nlohmann::ordered_json &calculateData,
                                      const std::string &date,
                                      bool verbose){
  

  std::string homeCountry;
  std::string firmCountry;
  JsonFunctions::getJsonString(countryData["home_country"],homeCountry);
  JsonFunctions::getJsonString(countryData["firm_country"],firmCountry);

  double inflation=JsonFunctions::getJsonFloat(
       calculateData[date]["costOfEquityAsAPercentage_inflation"],true);
  std::string inflationStr = formatJsonEntry(inflation);

  double inflationReference = JsonFunctions::getJsonFloat(
       calculateData[date]["costOfEquityAsAPercentage_inflationReference"],true);
  std::string inflationReferenceStr = formatJsonEntry(inflationReference);


  latexReport << "\\begin{tabular}{l l}" << std::endl;
  latexReport << "\\multicolumn{2}{c}{\\textbf{Cost of Capital}} \\\\" 
              << std::endl;

  latexReport << "\\multicolumn{2}{c}{"<<  date <<"} \\\\" << std::endl;
  latexReport << "\\hline \\multicolumn{2}{c}{Part 1: After-tax cost of debt} \\\\" 
                  << std::endl;
  latexReport << "\\hline ";

/*"
.*/

  latexReport << "$A_1$: Risk-free-rate (\\ref{data:risktable},"
                 "\\ref{data:fredbond},\\ref{data:userinput}) & "   
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfEquityAsAPercentage_riskFreeRate"],true))
              << " \\\\" << std::endl;
  latexReport << " & \\\\" << std::endl;
  latexReport << "\\multicolumn{2}{c}{ After-tax cost of debt } \\\\" << std::endl;
  latexReport << "\\hline "
              << "$B_1$: Operating income & "
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["interestCover_operatingIncome"],true))
              << " \\\\" << std::endl;
  latexReport << "$C_1$: Interest expense & "
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["interestCover_interestExpense"],true))
              << " \\\\" << std::endl;
  latexReport << "$D_1$: Interest cover & "
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["interestCover"],true))
              << " \\\\" << std::endl;
  latexReport << "\\multicolumn{2}{c}{ $G_1 = E_1/F_1$} \\\\" << std::endl;
  latexReport << "$E_1$. Default spread (\\ref{data:risktable},"
                 "\\ref{data:defaultSpread}) & "
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["defaultSpread"],true))
              << " \\\\" << std::endl;
  latexReport << "$F_1$. Tax rate (\\ref{data:taxfoundation},"
                 "\\ref{data:risktable},\\ref{data:userinput}) & "
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["afterTaxCostOfDebt_taxRate"],true))
              << " \\\\" << std::endl;
  latexReport << "$G_1$. After-tax cost of debt & "
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["afterTaxCostOfDebt"],true))
              << " \\\\" << std::endl;
  latexReport << "\\multicolumn{2}{c}{ $K_1 = (A_1+E_1)\\,(1.0-G_1)$} \\\\" 
              << std::endl;    
  latexReport << " & \\\\" << std::endl;
  latexReport << "\\multicolumn{2}{c}{ Cost of equity } \\\\" << std::endl;
  latexReport << "\\hline "
              << "$H_1$: equity risk premium (\\ref{data:risktable},\\ref{data:userinput})& " 
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfEquityAsAPercentage_equityRiskPremium"],true))
              << " \\\\" << std::endl;
  latexReport << "$I_1$: beta unleveraged & " 
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfEquityAsAPercentage_betaUnlevered"],true))
              << " \\\\" << std::endl;
  latexReport << "$J_1$: long term debt & " 
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfEquityAsAPercentage_longTermDebt"],true))
              << " \\\\" << std::endl;
  latexReport << "$K_1$: market capitalization & " 
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfEquityAsAPercentage_marketCapitalization"],true))
              << " \\\\" << std::endl;
  latexReport << "$L_1$: beta & " 
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfEquityAsAPercentage_beta"],true))
              << " \\\\" << std::endl;
  latexReport << "\\multicolumn{2}{c}{ $L_1 = I_1 \\,(1+ (1-F_1)(J_1/K_1))$} \\\\" << std::endl;            
  latexReport << "$M_1$: cost of equity (ignoring inflation)& " 
              <<formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfEquityAsAPercentage_noInflation"],true))
              << " \\\\" << std::endl;
  latexReport << "\\multicolumn{2}{c}{ $D_1 = A_1 + H_1\\,L_1$} \\\\" << std::endl;  
  latexReport << "$N_1$: Inflation rate ("
              << firmCountry
              << ") \\ref{data:risktable}& " 
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfEquityAsAPercentage_inflation"],true))
              << " \\\\" << std::endl;
  latexReport << "$O_1$: Inflation rate ("
              << homeCountry 
              << ") \\ref{data:risktable} & " 
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfEquityAsAPercentage_inflationReference"],true))
              << " \\\\" << std::endl;
  latexReport << "$P_1$: cost of equity & " 
              <<formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfEquityAsAPercentage"],true))
              << " \\\\" << std::endl;
  latexReport << "\\multicolumn{2}{c}{ $P_1 = M_1\\,(1+N_1)/(1+O_1)$} \\\\" << std::endl;
  latexReport << " & \\\\" << std::endl;              
  latexReport << "\\end{tabular}" << std::endl 
              << "\\bigskip" << std::endl<< std::endl;

  //Part II
  latexReport << "\\begin{tabular}{l l}" << std::endl;
  latexReport << "\\hline \\multicolumn{2}{c}{Part 2: Cost of capital} \\\\" 
              << std::endl;
  latexReport << "\\hline " << std::endl;                
  latexReport << "$A_2$. Outstanding shares & "
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfCapital_outstandingShares"],true))
              << " \\\\" << std::endl;                
  latexReport << "$B_2$. Adjusted close price & "
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfCapital_adjustedClose"],true))
              << " \\\\" << std::endl;                
  latexReport << "$C_2$. Market capitalization & "
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfCapital_marketCapitalization"],true))
              << " \\\\" << std::endl;
  latexReport << "\\multicolumn{2}{c}{ $C_2 =A_2\\,B_2$} \\\\" 
              << std::endl;              
  latexReport << "$D_2$. Long term debt  & "
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfCapital_longTermDebt"],true))
              << " \\\\" << std::endl;
  latexReport << "$E_2$. Cost of capital & "
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfCapital"],true))
              << " \\\\" << std::endl;
  latexReport << "\\multicolumn{2}{c}{ $E_2 = (P_1\\,C_2 + G_1\\,D_2)/(C_2+D_2)$} \\\\" 
              << std::endl;              
  latexReport << "$F_2$. Mature firm debt capital fraction \\ref{data:userinput} & "
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfCapitalMature_matureFirmDebtCapitalFraction"],true))
              << " \\\\" << std::endl;   
  latexReport << "$G_2$. Mature firm cost of capital & "
              << formatJsonEntry(JsonFunctions::getJsonFloat(
                  calculateData[date]["costOfCapitalMature"],true))
              << " \\\\" << std::endl;   
  latexReport << "\\multicolumn{2}{c}{ $G_2 = D_1\\,(1-F_2)+K_1\\,F_2 $} \\\\" 
              << std::endl;              
  latexReport << "\\end{tabular}" << std::endl 
              << "\\bigskip" << std::endl<< std::endl;  

  latexReport << "\\center{Data sources (update annually)}" << std::endl;
  latexReport << "\\hrule" << std::endl;
  latexReport << "\\begin{enumerate}[noitemsep,nolistsep]" << std::endl;  
  latexReport << "\\footnotesize" << std::endl;
  latexReport << "\\item \\href{https://aswathdamodaran.blogspot.com/2024/07/"
                "country-risk-my-2024-data-update.html}"
              << "{Prof. Damodaran's 2024 country risk table}.\\\\ The PRS "
              "Worksheet data from ctrypremJuly24.xlsx was "
              "converted to json using src/csvTools and src/jsonTools"
              " \\label{data:risktable}"
              << std::endl;
  latexReport << "\\item \\href{https://fred.stlouisfed.org/series/DGS10}{FRED US "
                "10y bond yield historical record} \\label{data:fredbond}"
              << std::endl;
  latexReport << "\\item Default values (user input) \\label{data:userinput}" 
              << std::endl;
  latexReport << "\\item \\href{https://pages.stern.nyu.edu/~adamodar/New_Home"
                 "_Page/datafile/ratings.html}"
              << "{Prof. Damodaran's synthetic default spread table (USA)}. \\\\"
                 " 2023-12-31 \\label{data:defaultSpread}"
              << std::endl;
  latexReport << "\\item \\href{https://taxfoundation.org/data/all/global/"
                 "corporate-tax-rates-by-country-2023/}"
              << "{taxfoundation.org tax table)} \\\\ Historical comporate tax "
                 "rates by country (1980-2023 \\label{data:taxfoundation}"
              << std::endl;
  latexReport << "\\end{enumerate}" 
              << std::endl;
  latexReport << "\\bigskip " << std::endl << std::endl;  

};
//==============================================================================
// The 'ForValuation' ending means these tables cross reference other
// tables, and so, each table has a specific number
static void appendReinvestmentRateTableForValuation(std::ofstream &latexReport, 
                           const std::string &primaryTicker, 
                           const nlohmann::ordered_json &calculateData,
                           const std::string &date,
                           bool verbose){

  latexReport << std::endl << "\\bigskip" << std::endl<< std::endl;

    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\multicolumn{2}{c}{\\textbf{Reinvestment Rate}} \\\\" 
                << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part 3: Net Capital Expenditures} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;    
    latexReport << "$A_3$. Capital expenditures &"
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["reinvestmentRate_netCapitalExpenditures_capitalExpenditures"],true))
                << " \\\\" << std::endl;                              
    latexReport << "$B_3$. $\\Delta$ property plant \\& equip. (PPE) &"
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["reinvestmentRate_netCapitalExpenditures_changeInPlantPropertyEquipment"],true))
                << " \\\\" << std::endl;   
    latexReport << "\\multicolumn{2}{c}{If cap. exp. is 0 in EOD's records:} \\\\" 
                << std::endl;                
    latexReport << "\\multicolumn{2}{c}{ $A_3 = B_3$} \\\\" 
                << std::endl;                
    latexReport << "$C_3$. Depreciation & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["reinvestmentRate_netCapitalExpenditures_depreciation"],true))
                << " \\\\" << std::endl;
    latexReport << "$D_3$. Net Capital Expenditures & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["reinvestmentRate_netCapitalExpenditures"],true))
                << " \\\\" << std::endl;                
    latexReport << "\\multicolumn{2}{c}{ $D_3 = A_3 - C_3$ } \\\\" 
                << std::endl;              
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;  

    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part 4: Change in non-cash working capital} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;                
    latexReport << "$A_4$. $\\Delta$ Inventory & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["reinvestmentRate_changeInNonCashWorkingCapital_changeInInventory"],true))
                << " \\\\" << std::endl;                 
    latexReport << "$B_4$. $\\Delta$ Net receivables & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["reinvestmentRate_changeInNonCashWorkingCapital_changeInNetReceivables"],true))
                << " \\\\" << std::endl;                             
    latexReport << "$C_4$. $\\Delta$ Accounts payable & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["reinvestmentRate_changeInNonCashWorkingCapital_changeInAccountsPayable"],true))
                << " \\\\" << std::endl;                                         
    latexReport << "$D_4$. $\\Delta$ Non-cash working cap. & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["reinvestmentRate_changeInNonCashWorkingCapital"],true))
                << " \\\\" << std::endl;                                                 
    latexReport << "\\multicolumn{2}{c}{ $D_4 = A_4+B_4-C_4$} \\\\" 
                << std::endl;                  
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;   


    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part 5: Reinvestment rate} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;                
    latexReport << "$A_5$. Operating income & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["reinvestmentRate_operatingIncome"],true))
                << " \\\\" << std::endl;
    latexReport << "$B_5$. After tax operating income & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["reinvestmentRate_afterTaxOperatingIncome"],true))
                << " \\\\" << std::endl;                  
    latexReport << "\\multicolumn{2}{c}{ $B_5 = A_5\\,(1.0-J_1)$} \\\\" 
                << std::endl;                
    latexReport << "$C_5$. Reinvestment rate & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["reinvestmentRate"],true))
                << " \\\\" << std::endl;                  
    latexReport << "\\multicolumn{2}{c}{ $C_5 = (D_3+D_4)/B_5$} \\\\" 
                << std::endl;                
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;

};
//==============================================================================
static void appendReturnOnInvestedCapitalTableForValuation(std::ofstream &latexReport, 
                           const std::string &primaryTicker, 
                           const nlohmann::ordered_json &calculateData,
                           const std::string &date,
                           bool verbose){

    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\multicolumn{2}{c}{\\textbf{Return on invested capital (Finance)}} \\\\" 
                << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part 6} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;                    
    latexReport << "$A_6$. Debt book value & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital_debtBookValue"],true))
                << " \\\\" << std::endl; 
    latexReport << "\\qquad short-long term debt & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital_shortLongTermDebt"],true))
                << " \\\\" << std::endl; 
    latexReport << "\\qquad long term debt & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital_longTermDebt"],true))
                << " \\\\" << std::endl; 
    latexReport << "$B_6$. Total stock holder equity & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital_totalStockholderEquity"],true),3,
                    true)
                << " \\\\" << std::endl;
    latexReport << "$C_6$. Cash & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital_cash"],true))
                << " \\\\" << std::endl;
    latexReport << "$D_6$. After tax operating income & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital_afterTaxOperatingIncome"],true))
                << " \\\\" << std::endl;           
    latexReport << "\\multicolumn{2}{c}{ $D_6 = A_5\\,(1.0-J_1)$} \\\\" 
                << std::endl;                       
    latexReport << "$E_6$. Return on invested capital & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["returnOnInvestedFinancialCapital"],true))
                << " \\\\" << std::endl;                 
    latexReport << "\\multicolumn{2}{c}{ $E_6 = D_6/(A_6+B_6-C_6)$} \\\\" 
                << std::endl;  
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;  

};
//==============================================================================
static void appendOperatingIncomeGrowthTableForValuation(std::ofstream &latexReport, 
                           const std::string &primaryTicker, 
                           const nlohmann::ordered_json &calculateData,
                           const std::string &date,
                           bool verbose){

    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\multicolumn{2}{c}{\\textbf{Operating income growth}} \\\\" 
                << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part 7} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;                
    latexReport << "$A_7$. After tax op. income growth & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["afterTaxOperatingIncomeGrowth"],true))
                << " \\\\" << std::endl;  
    latexReport << "\\multicolumn{2}{c}{ $A_7 = C_5\\,E_6$} \\\\" 
            << std::endl;                            
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;                


};
//==============================================================================
static int appendValuationTable(std::ofstream &latexReport, 
                           const std::string &primaryTicker, 
                           const nlohmann::ordered_json &calculateData,
                           const std::string &date,
                           int tableId,
                           const std::string &tableTitle,
                           const std::string &jsonTableName,
                           bool verbose){


  if(calculateData[date].contains(jsonTableName+"_riskFreeRate")){
    //latexReport << "\\begin{table}[h]" << std::endl;
    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\multicolumn{2}{c}{\\textbf{" << tableTitle <<"}} \\\\" 
                << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part " << tableId << ": Growth inputs} \\\\" 
                << std::endl; 
    latexReport << "\\hline " << std::endl;                               
    latexReport << "$A_{" << tableId << "}^0$. Return on invested capital & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date][jsonTableName+"_returnOnInvestedCapital"],true))
                << " \\\\" << std::endl;
    latexReport << "$B_{" << tableId << "}^0$. Reinvestment rate & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date][jsonTableName+"_reinvestmentRate"],true))
                << " \\\\" << std::endl;                            
    latexReport << "$C_{" << tableId << "}^0$. After-tax op. income growth & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date][jsonTableName+"_afterTaxOperatingIncomeGrowth"],true))
                << " \\\\" << std::endl;

    ++tableId;

    int year = 0;
    std::stringstream atoiFieldName, rrFieldName, fcfFieldName;
    atoiFieldName << jsonTableName << "_afterTaxOperatingIncome_" << year;
    rrFieldName   << jsonTableName << "_reinvestment_" << year;
    fcfFieldName  << jsonTableName << "_freeCashFlowToFirm_" << year;

    latexReport << "  & \\\\" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part " 
                << tableId 
                << ": Income during growth period} \\\\" 
                << std::endl; 

    while(  calculateData[date].contains(atoiFieldName.str().c_str()) 
            && calculateData[date].contains(rrFieldName.str().c_str())
            && calculateData[date].contains(fcfFieldName.str().c_str())
        ){


        latexReport << "\\hline " << std::endl;                               
        latexReport << "$A_{" << tableId << "}^"<< year 
                    <<"$. After-tax op. income (year "<< year <<") & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date][atoiFieldName.str().c_str()],true))
                    << " \\\\" << std::endl;
        latexReport << "$B_{" << tableId << "}^" << year 
                    << "$. Reinvestment (year "<< year <<") & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date][rrFieldName.str().c_str()],true))
                    << " \\\\" << std::endl;
        latexReport << "$C_{" << tableId << "}^" << year 
                    << "$. Free cash flow to firm  (year "<< year <<") & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                        calculateData[date][fcfFieldName.str().c_str()],true))
                    << " \\\\" << std::endl;

        ++year;

        atoiFieldName.str(std::string());
        rrFieldName.str(std::string());
        fcfFieldName.str(std::string());

        atoiFieldName << jsonTableName << "_afterTaxOperatingIncome_" << year;
        rrFieldName   << jsonTableName << "_reinvestment_" << year;
        fcfFieldName  << jsonTableName << "_freeCashFlowToFirm_" << year;
    }
    --year;


    latexReport << "\\hline ";
    latexReport << "\\multicolumn{2}{c}{ $A_{" << tableId << "}^{i+1} = A_{" << tableId << "}^{i}\\,(1+A_7)$} \\\\" 
                << std::endl;
    latexReport << "\\multicolumn{2}{c}{ $B_{" << tableId << "}^{i+1} = A_{" << tableId << "}^{i+1}\\,C_5$} \\\\" 
                << std::endl; 
    latexReport << "\\multicolumn{2}{c}{ $C_{" << tableId << "}^{i+1} = A_{" << tableId << "}^{i+1}-B_{" << tableId << "}^{i+1}$} \\\\" 
                << std::endl;              

    ++tableId;

    latexReport << "  & \\\\" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part "
                <<tableId<<": Present value of DCM} \\\\" 
                << std::endl;
    latexReport << "\\hline ";
    //Inputs  (restated)
    latexReport << "$A_{1}$. Risk free rate (terminal) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]
                    [jsonTableName+"_terminalValue_riskFreeRate"],true))
                << " \\\\" << std::endl;    

    latexReport << "$E_{2}$. Cost of capital & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]
                    [jsonTableName+"_terminalValue_costOfCapital"],true))
                << " \\\\" << std::endl;  

    latexReport << "$G_{2}$. Cost of capital (mature) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]
                    [jsonTableName+"_terminalValue_costOfCapitalMature"],true))
                << " \\\\" << std::endl; 

    latexReport << "\\hline ";
    latexReport << "$A_{" << tableId << "}$. Reinvestment rate  & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]
                    [jsonTableName+"_terminalValue_reinvestmentRateStableGrowth"],true))
                << " \\\\" << std::endl;    
    latexReport << "\\qquad stable growth (terminal) & "
                << " \\\\" << std::endl;    
    latexReport << "\\multicolumn{2}{c}{ $A_{" << tableId << "}=  A_{1}/G_{2}$ }\\\\" 
                << std::endl; 

    latexReport << "$B_{" << tableId << "}$. After tax op. income (terminal) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]
                    [jsonTableName+"_terminalValue_afterTaxOperatingIncome"],true))
                << " \\\\" << std::endl;   

    latexReport << "$C_{" << tableId << "}$. Terminal value (year "<<year<<") & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date][jsonTableName+"_terminalValue"],true))
                << " \\\\" << std::endl;  

    latexReport << "\\multicolumn{2}{c}{ $C_{" << tableId << "}=B_{" 
                << tableId << "}\\,(1+A_{1})\\,(1-A_{" 
                << tableId << "})/(G_{2}-A_{1})$} \\\\" 
                << std::endl; 
    latexReport << "$D_{" << tableId << "}$. Present value of DCF & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date][jsonTableName+"_presentValueOfFutureCashFlows"],true))
                << " \\\\" << std::endl; 

    latexReport << "\\multicolumn{2}{c}{ $D_{" 
                << tableId << "}=\\sum_{i=0}^{"<<year<<"} C_{" 
                << tableId << "}^i/(1+G_{2})^i + C_{" 
                << tableId << "}/(1+G_{2})^5$} \\\\" 
                << std::endl; 
    latexReport << "\\end{tabular}" << std::endl; 
    //latexReport << "\\end{table}" << std::endl; 
    latexReport << "\\bigskip" << std::endl<< std::endl;  


    ++tableId;

    //latexReport << "\\begin{table}[h]" << std::endl;
    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part "
                << tableId <<": Price to value} \\\\" 
                << std::endl;
    latexReport << "\\hline";               
    latexReport << "$A_{" << tableId << "}$. Cash balance & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date][jsonTableName+"_cash"],true))
                << " \\\\" << std::endl; 
    latexReport << "$B_{" << tableId << "}$. Value of minority holdings & ? "
                << " \\\\" << std::endl; 
    latexReport << "$C_{" << tableId << "}$. Value of minority interests & ? "
                << " \\\\" << std::endl; 
    latexReport << "(from majority stake) &  "
                << " \\\\" << std::endl; 
    latexReport << "$D_{" << tableId << "}$. Short and long term debt  &  "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date][jsonTableName+"_shortLongTermDebtTotalEntry"],true))
                << " \\\\" << std::endl;                 
    latexReport << "\\qquad Short and long term debt entry &  "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date][jsonTableName+"_shortLongTermDebtTotal"],true))
                << " \\\\" << std::endl;                 
    latexReport << "\\qquad long term debt entry &  "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date][jsonTableName+"_longTermDebt"],true))
                << " \\\\" << std::endl;                 

    latexReport << "$E_{" << tableId << "}$. Potential liabilities  & ? "
                << " \\\\" << std::endl; 
    latexReport << "(underfunded pension, lawsuits ...) &  "
                << " \\\\" << std::endl; 
    latexReport << "$F_{" << tableId << "}$. Value of managment options  & ? "
                << " \\\\" << std::endl; 
    latexReport << "$G_{" << tableId << "}$. Present value approx.  & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date][jsonTableName+"_presentValue"],true))
                << " \\\\" << std::endl; 
    latexReport << "\\multicolumn{2}{c}{ $G_{" << tableId 
                  << "}= D_{" << (tableId-1) << "}+A_{" << tableId 
                  << "}-B_{" << tableId << "}-C_{" << tableId 
                  << "}-D_{" << tableId << "}-E_{" << tableId 
                  << "}-F_{" << tableId << "}$} \\\\" 
                << std::endl; 
    latexReport << "$C_{2}$. Market cap. & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date][jsonTableName+"_marketCapitalization"],true))
                << " \\\\" << std::endl; 
    latexReport << "$H_{" << tableId << "}$. Price / value & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date][jsonTableName],true))
                << " \\\\" << std::endl; 
    latexReport << "\\multicolumn{2}{c}{ $G_{2}= C_{2} / F_{" << tableId << "}$} \\\\" 
                << std::endl; 
    latexReport << "\\end{tabular}" << std::endl;
       
  }
   return tableId;                
};

//==============================================================================



};


#endif
