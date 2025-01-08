//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT

#ifndef REPORTING_FUNCTIONS
#define REPORTING_FUNCTIONS

#include <vector>

#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

#include "JsonFunctions.h"


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
  deleteCharacters(stringForLatex,charactersToDelete);

  std::string charactersToEscape("");
  charactersToEscape.append("&");
  charactersToEscape.append("$");
  charactersToEscape.append("#");
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
static std::string formatJsonEntry( double entry){


  std::stringstream ss;
  if(JsonFunctions::isJsonFloatValid(entry)){
    ss << entry;
  }else{
    ss << "\\cellcolor{RedOrange}" << std::round(entry);
  }
  
  return ss.str();

}


//==============================================================================
static void appendValuationTable(std::ofstream &latexReport, 
                           const std::string &primaryTicker, 
                           const nlohmann::ordered_json &calculateData,
                           const std::string &date,
                           bool verbose){

    latexReport << std::endl << std::endl << "\\bigskip" << std::endl;
    latexReport << "Inputs are indicated using (*). Note the constant equity"
                << " risk premium should be replaced with " 
                << "\\href{https://aswathdamodaran.blogspot.com/2024/07/country-risk-my-2024-data-update.html}{Prof. Damodaran's country risk} method."
                << "  The values of 0.05 are appropriate for safe developed "
                << " countries: not all countries in EOD's database are safe" 
                << " and developed. ";
    latexReport << std::endl << "\\bigskip" << std::endl<< std::endl;

    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\multicolumn{2}{c}{\\textbf{Discounted Cashflow Value}} \\\\" 
                    << std::endl;
    latexReport << "\\multicolumn{2}{c}{"<<  date <<"} \\\\" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part 1: After-tax cost of debt} \\\\" 
                    << std::endl;
    latexReport << "\\hline ";
    latexReport << "$A_1$: Risk-free-rate [1] & " 
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["costOfEquityAsAPercentage_riskFreeRate"],true))
                << " \\\\" << std::endl;
    latexReport << " & \\\\" << std::endl;
    latexReport << "\\multicolumn{2}{c}{ Cost of equity } \\\\" << std::endl;
    latexReport << "\\hline "
                << "$B_1$: equity risk premium* & " 
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["costOfEquityAsAPercentage_equityRiskPremium"],true))
                << " \\\\" << std::endl;
    latexReport << "$C_1$: beta & " 
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["costOfEquityAsAPercentage_beta"],true))
                << " \\\\" << std::endl;
    latexReport << "$D_1$: cost of equity & " 
                <<formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["costOfEquityAsAPercentage"],true))
                << " \\\\" << std::endl;
    latexReport << "\\multicolumn{2}{c}{ $D_1 = A_1 + B_1\\,C_1$} \\\\" << std::endl;
    latexReport << " & \\\\" << std::endl;

    latexReport << "\\multicolumn{2}{c}{ After-tax cost of debt } \\\\" << std::endl;
    latexReport << "\\hline "
                << "$E_1$: Operating income & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["interestCover_operatingIncome"],true))
                << " \\\\" << std::endl;
    latexReport << "$F_1$: Interest expense & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["interestCover_interestExpense"],true))
                << " \\\\" << std::endl;
    latexReport << "$G_1$: Interest cover & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["interestCover"],true))
                << " \\\\" << std::endl;
    latexReport << "\\multicolumn{2}{c}{ $G_1 = E_1/F_1$} \\\\" << std::endl;
    latexReport << " & \\\\" << std::endl;
    latexReport << "$I_1$. Default spread [2] & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["defaultSpread"],true))
                << " \\\\" << std::endl;
    latexReport << "$J_1$. Tax rate [3] & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["afterTaxCostOfDebt_taxRate"],true))
                << " \\\\" << std::endl;
    latexReport << "$K_1$. After-tax cost of debt & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["afterTaxCostOfDebt"],true))
                << " \\\\" << std::endl;
    latexReport << "\\multicolumn{2}{c}{ $K_1 = (A_1+I_1)\\,(1.0-J_1)$} \\\\" 
                << std::endl;    
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
    latexReport << "\\multicolumn{2}{c}{ $E_2 = (D_1\\,C_2 + K_1\\,D_2)/(C_2+D_2)$} \\\\" 
                << std::endl;              
    latexReport << "$F_2$. Mature firm debt capital fraction* & "
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


    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part 3: Net Capital Expenditures} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;    
    latexReport << "$A_3$. Capital expenditures &"
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_netCapitalExpenditures_capitalExpenditures"],true))
                << " \\\\" << std::endl;                              
    latexReport << "$B_3$. $\\Delta$ property plant \\& equip. (PPE) &"
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_netCapitalExpenditures_changeInPlantPropertyEquipment"],true))
                << " \\\\" << std::endl;   
    latexReport << "\\multicolumn{2}{c}{If cap. exp. is 0 in EOD's records:} \\\\" 
                << std::endl;                
    latexReport << "\\multicolumn{2}{c}{ $A_3 = B_3$} \\\\" 
                << std::endl;                
    latexReport << "$C_3$. Depreciation & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_netCapitalExpenditures_depreciation"],true))
                << " \\\\" << std::endl;
    latexReport << "$D_3$. Net Capital Expenditures & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_netCapitalExpenditures"],true))
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
                    calculateData[date]["presentValueDCF_reinvestmentRate_changeInNonCashWorkingCapital_changeInInventory"],true))
                << " \\\\" << std::endl;                 
    latexReport << "$B_4$. $\\Delta$ Net receivables & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_changeInNonCashWorkingCapital_changeInNetReceivables"],true))
                << " \\\\" << std::endl;                             
    latexReport << "$C_4$. $\\Delta$ Accounts payable & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_changeInNonCashWorkingCapital_changeInAccountsPayable"],true))
                << " \\\\" << std::endl;                                         
    latexReport << "$D_4$. $\\Delta$ Non-cash working cap. & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_changeInNonCashWorkingCapital"],true))
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
                    calculateData[date]["presentValueDCF_reinvestmentRate_operatingIncome"],true))
                << " \\\\" << std::endl;
    latexReport << "$B_5$. After tax operating income & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_afterTaxOperatingIncome"],true))
                << " \\\\" << std::endl;                  
    latexReport << "\\multicolumn{2}{c}{ $B_5 = A_5\\,(1.0-J_1)$} \\\\" 
                << std::endl;                
    latexReport << "$C_5$. Reinvestment rate & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate"],true))
                << " \\\\" << std::endl;                  
    latexReport << "\\multicolumn{2}{c}{ $C_5 = (D_3+D_4)/B_5$} \\\\" 
                << std::endl;                
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;

    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part 6: Return on invested capital} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;                
    latexReport << "$A_6$. Long term debt & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_returnOnInvestedCapital_longTermDebt"],true))
                << " \\\\" << std::endl; 
    latexReport << "$B_6$. Total stock holder equity & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_returnOnInvestedCapital_totalStockholderEquity"],true))
                << " \\\\" << std::endl;
    latexReport << "$C_6$. Net income & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_returnOnInvestedCapital_netIncome"],true))
                << " \\\\" << std::endl;                 
    latexReport << "$D_6$. Dividends paid & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_returnOnInvestedCapital_dividendsPaid"],true))
                << " \\\\" << std::endl;                 
    latexReport << "$E_6$. Return on invested capital & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_returnOnInvestedCapital"],true))
                << " \\\\" << std::endl;                 
    latexReport << "\\multicolumn{2}{c}{ $E_6 = (C_6-D_6)/(A_6+B_6)$} \\\\" 
                << std::endl;  
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;  


    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part 7: Operating income growth} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;                
    latexReport << "$A_7$. Operating income growth & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_operatingIncomeGrowth"],true))
                << " \\\\" << std::endl;  
    latexReport << "\\multicolumn{2}{c}{ $A_7 = C_5\\,E_6$} \\\\" 
            << std::endl;                            
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;                

    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part 8: Retention ratio} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;                
    latexReport << "$A_8$. Net income & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_retentionRatio_netIncome"],true))
                << " \\\\" << std::endl;
    latexReport << "$B_8$. Dividends paid & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_retentionRatio_dividendsPaid"],true))
                << " \\\\" << std::endl;
    latexReport << "$C_8$. Retention ratio & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_retentionRatio"],true))
                << " \\\\" << std::endl;                
    latexReport << "\\multicolumn{2}{c}{ $C_8 = (A_8-B_8)/A_8$} \\\\" 
            << std::endl;   
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;                


    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part 9: Return on equity} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;                
    latexReport << "$A_9$. Net income & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_returnOnEquity_netIncome"],true))
                << " \\\\" << std::endl;
    latexReport << "$B_9$. Total stock holder equity & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_returnOnEquity_totalStockholderEquity"],true))
                << " \\\\" << std::endl;
    latexReport << "$C_9$. Return on equity & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_returnOnEquity"],true))
                << " \\\\" << std::endl;                
    latexReport << "\\multicolumn{2}{c}{ $C_9 = A_9/B_9$} \\\\" 
            << std::endl;   
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl; 


    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part 10: Net income growth} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;                
    latexReport << "$A_{10}$. Net income growth & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_netIncomeGrowth"],true))
                << " \\\\" << std::endl;                
    latexReport << "\\multicolumn{2}{c}{ $A_{10} = C_8\\,C_9$} \\\\" 
            << std::endl;  
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;                

    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part 11: Income during growth period} \\\\" 
                << std::endl;
    latexReport << "\\hline ";
    latexReport << "$A_{11}^0$. After-tax op. income (year 0) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_afterTaxOperatingIncome_0"],true))
                << " \\\\" << std::endl;
    latexReport << "$B_{11}^0$. Reinvestment (year 0) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestment_0"],true))
                << " \\\\" << std::endl;
    latexReport << "$C_{11}^0$. Free cash flow to firm  (year 0) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_freeCashFlowToFirm_0"],true))
                << " \\\\" << std::endl;

    latexReport << "\\hline ";
    latexReport << "$A_{11}^1$. After-tax op. income (year 1) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_afterTaxOperatingIncome_1"],true))
                << " \\\\" << std::endl;
    latexReport << "$B_{11}^1$. Reinvestment (year 1) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestment_1"],true))
                << " \\\\" << std::endl;
    latexReport << "$C_{11}^1$. Free cash flow to firm  (year 1) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_freeCashFlowToFirm_1"],true))
                << " \\\\" << std::endl;

    latexReport << "\\hline ";
    latexReport << "$A_{11}^2$. After-tax op. income (year 2) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_afterTaxOperatingIncome_2"],true))
                << " \\\\" << std::endl;
    latexReport << "$B_{11}^2$. Reinvestment (year 2) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestment_2"],true))
                << " \\\\" << std::endl;
    latexReport << "$C_{11}^2$. Free cash flow to firm  (year 2) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_freeCashFlowToFirm_2"],true))
                << " \\\\" << std::endl;

    latexReport << "\\hline ";
    latexReport << "$A_{11}^3$. After-tax op. income (year 3) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_afterTaxOperatingIncome_3"],true))
                << " \\\\" << std::endl;
    latexReport << "$B_{11}^3$. Reinvestment (year 3) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestment_3"],true))
                << " \\\\" << std::endl;
    latexReport << "$C_{11}^3$. Free cash flow to firm  (year 3) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_freeCashFlowToFirm_3"],true))
                << " \\\\" << std::endl;

    latexReport << "\\hline ";
    latexReport << "$A_{11}^4$. After-tax op. income (year 4) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_afterTaxOperatingIncome_4"],true))
                << " \\\\" << std::endl;
    latexReport << "$B_{11}^4$. Reinvestment (year 4) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestment_4"],true))
                << " \\\\" << std::endl;
    latexReport << "$C_{11}^4$. Free cash flow to firm  (year 4) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_freeCashFlowToFirm_4"],true))
                << " \\\\" << std::endl;

    latexReport << "\\hline ";
    latexReport << "$A_{11}^5$. After-tax op. income (year 5) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_afterTaxOperatingIncome_5"],true))
                << " \\\\" << std::endl;
    latexReport << "$B_{11}^5$. Reinvestment (year 5) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestment_5"],true))
                << " \\\\" << std::endl;
    latexReport << "$C_{11}^5$. Free cash flow to firm  (year 5) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_freeCashFlowToFirm_5"],true))
                << " \\\\" << std::endl;                
    latexReport << "\\hline ";
    latexReport << "\\multicolumn{2}{c}{ $A_{11}^{i+1} = A_{11}^{i}\\,(1+A_7)$} \\\\" 
                << std::endl;
    latexReport << "\\multicolumn{2}{c}{ $B_{11}^{i+1} = A_{11}^{i+1}\\,C_5$} \\\\" 
                << std::endl; 
    latexReport << "\\multicolumn{2}{c}{ $C_{11}^{i+1} = A_{11}^{i+1}-B_{11}^{i+1}$} \\\\" 
                << std::endl; 
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;                


    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part 12: Present value of DCM} \\\\" 
                << std::endl;
    latexReport << "\\hline ";

    //Inputs  (restated)
    latexReport << "$A_{1}$. Risk free rate (terminal) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_terminalValue_riskFreeRate"],true))
                << " \\\\" << std::endl;    

    latexReport << "$E_{2}$. Cost of capital & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_terminalValue_costOfCapital"],true))
                << " \\\\" << std::endl;  

    latexReport << "$G_{2}$. Cost of capital (mature) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_terminalValue_costOfCapitalMature"],true))
                << " \\\\" << std::endl; 

    latexReport << "\\hline ";
    latexReport << "$A_{12}$. Reinvestment rate  & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_terminalValue_reinvestmentRateStableGrowth"],true))
                << " \\\\" << std::endl;    
    latexReport << "  stable growth (terminal) & "
                << " \\\\" << std::endl;    
    latexReport << "\\multicolumn{2}{c}{ $A_{12}=  A_{1}/G_{2}$ }\\\\" 
                << std::endl; 

    latexReport << "$B_{12}$. After tax op. income (terminal) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_terminalValue_afterTaxOperatingIncome"],true))
                << " \\\\" << std::endl;   

    latexReport << "$C_{12}$. Terminal value (year 5) & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_terminalValue"],true))
                << " \\\\" << std::endl;  

    latexReport << "\\multicolumn{2}{c}{ $C_{12}=B_{12}\\,(1+A_{1})\\,(1-A_{12})/(G_{2}-A_{1})$} \\\\" 
                << std::endl; 
    latexReport << "$D_{12}$. Present value of DCF & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF"],true))
                << " \\\\" << std::endl; 

    latexReport << "\\multicolumn{2}{c}{ $D_{12}=\\sum_{i=0}^{5} C_{11}^i/(1+G_{2})^i + C_{12}/(1+G_{2})^5$} \\\\" 
                << std::endl; 
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;  



    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part 13: Price to value} \\\\" 
                << std::endl;
    latexReport << "$A_{13}$. Cash balance & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["priceToValue_cash"],true))
                << " \\\\" << std::endl; 
    latexReport << "$B_{13}$. Value of minority holdings & ? "
                << " \\\\" << std::endl; 
    latexReport << "$C_{13}$. Value of minority interests & ? "
                << " \\\\" << std::endl; 
    latexReport << "(from majority stake) &  "
                << " \\\\" << std::endl; 
    latexReport << "$D_{13}$. Short and long term debt  &  "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["priceToValue_shortLongTermDebtTotal"],true))
                << " \\\\" << std::endl;                 
    latexReport << "$E_{13}$. Potential liabilities  & ? "
                << " \\\\" << std::endl; 
    latexReport << "(underfunded pension, lawsuits ...) &  "
                << " \\\\" << std::endl; 
    latexReport << "$F_{13}$. Value of managment options  & ? "
                << " \\\\" << std::endl; 
    latexReport << "$G_{13}$. Present value approx.  & "
                    << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["priceToValue_presentValue_approximation"],true))
                << " \\\\" << std::endl; 
    latexReport << "\\multicolumn{2}{c}{ $G_{13}= D_{12}+A_{13}-B_{13}-C_{13}-D_{13}-E_{13}-F_{13}$} \\\\" 
                << std::endl; 
    latexReport << "$C_{2}$. Market cap. & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["priceToValue_marketCapitalization"],true))
                << " \\\\" << std::endl; 
    latexReport << "$H_{13}$. Price / value & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["priceToValue"],true))
                << " \\\\" << std::endl; 
    latexReport << "\\multicolumn{2}{c}{ $G_{2}= C_{2} / F_{13}$} \\\\" 
                << std::endl; 


    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;  



    latexReport << "\\underline{References}" << std::endl;
    latexReport << "\\begin{enumerate}" << std::endl;
    latexReport << "\\item The U.S. 10 year bond rate is used to estimate "
                << "the risk-free-rate in all countries across time. This is"
                << " obviously only appropriate if you can actually purchase "
                << " US 10 year treasury bonds. \\\\" 
                << "\\href{https://fred.stlouisfed.org/series/DGS10}{Federal Reserve Bank of St. Louis}" 
                << std::endl;
    latexReport << "\\item " 
                << "\\href{https://pages.stern.nyu.edu/~adamodar/New_Home_Page/datafile/ratings.html}{Prof. Damodaran's Ratings File}" 
                << std::endl;           
    latexReport << "\\item " 
                << "\\href{https://taxfoundation.org/data/all/global/corporate-tax-rates-by-country-2023/}{Corporate Tax Rates By Country}" 
                << std::endl;                            
    latexReport << "\\end{enumerate}" << std::endl;

                                
};

//==============================================================================



};


#endif
