#ifndef REPORTING_FUNCTIONS
#define REPORTING_FUNCTIONS

#include <vector>

#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

#include "JsonFunctions.h"
#include "UtilityFunctions.h"


class ReportingFunctions {

public:

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
    latexReport << "$A_2$. Common shares outstanding & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["costOfCapital_commonStockSharesOutstanding"],true))
                << " \\\\" << std::endl;                
    latexReport << "$B_2$. Adjusted Close & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["costOfCapital_close"],true))
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
    latexReport << "$B_3$. Property plant \\& equip. (PPE) &"
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_netCapitalExpenditures_plantPropertyEquipment"],true))
                << " \\\\" << std::endl;   
    latexReport << "$C_3$. Previous PPE & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_netCapitalExpenditures_plantPropertyEquipmentPrevious"],true))
                << " \\\\" << std::endl;                
    latexReport << "\\multicolumn{2}{c}{If cap. exp. is 0 in EOD's records:} \\\\" 
                << std::endl;                
    latexReport << "\\multicolumn{2}{c}{ $A_3 = B_3 - C_3$} \\\\" 
                << std::endl;                
    latexReport << "$D_3$. Depreciation & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_netCapitalExpenditures_depreciation"],true))
                << " \\\\" << std::endl;                
    latexReport << "$E_3$. Net Capital Expenditures & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_netCapitalExpenditures"],true))
                << " \\\\" << std::endl;                
    latexReport << "\\multicolumn{2}{c}{ $E_3 = A_3 - D_3$} \\\\" 
                << std::endl;              
    latexReport << "\\end{tabular}" << std::endl 
                << "\\bigskip" << std::endl<< std::endl;  

    latexReport << "\\begin{tabular}{l l}" << std::endl;
    latexReport << "\\hline \\multicolumn{2}{c}{Part 4: Change in non-cash working capital} \\\\" 
                << std::endl;
    latexReport << "\\hline " << std::endl;                
    latexReport << "$A_4$. Inventory & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_changeInNonCashWorkingCapital_inventory"],true))
                << " \\\\" << std::endl;                 
    latexReport << "$B_4$. Previous inventory & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_changeInNonCashWorkingCapital_inventoryPrevious"],true))
                << " \\\\" << std::endl;
    latexReport << "$C_4$. Net receivables & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_changeInNonCashWorkingCapital_netReceivables"],true))
                << " \\\\" << std::endl;                             
    latexReport << "$D_4$. Previous net receivables & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_changeInNonCashWorkingCapital_netReceivablesPrevious"],true))
                << " \\\\" << std::endl;
    latexReport << "$E_4$. Accounts payable & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_changeInNonCashWorkingCapital_accountsPayable"],true))
                << " \\\\" << std::endl;                                         
    latexReport << "$F_4$. Previous accounts payable & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_changeInNonCashWorkingCapital_accountsPayablePrevious"],true))
                << " \\\\" << std::endl;        
    latexReport << "$G_4$. Change in non-cash  & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["presentValueDCF_reinvestmentRate_changeInNonCashWorkingCapital"],true))
                << " \\\\" << std::endl;                                                 

    latexReport << " working capital & "
                << " \\\\" << std::endl;                                                 
    latexReport << "\\multicolumn{2}{c}{ $G_4 = (A_4-B_4)+(C_4-D_4)-(E_4-F_4)$} \\\\" 
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
    latexReport << "\\multicolumn{2}{c}{ $C_5 = (E_3+G_4)/B_5$} \\\\" 
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
    latexReport << "$D_{13}$. Potential liabilities  & ? "
                << " \\\\" << std::endl; 
    latexReport << "(underfunded pension, lawsuits ...) &  "
                << " \\\\" << std::endl; 
    latexReport << "$E_{13}$. Value of managment options  & ? "
                << " \\\\" << std::endl; 
    latexReport << "$F_{13}$. Number of shares outstanding  & ? "
                << " \\\\" << std::endl; 


    latexReport << "$C_{2}$. Market cap. & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["priceToValue_marketCapitalization"],true))
                << " \\\\" << std::endl; 
    latexReport << "$G_{13}$. Price / value & "
                << formatJsonEntry(JsonFunctions::getJsonFloat(
                    calculateData[date]["priceToValue"],true))
                << " \\\\" << std::endl; 
    latexReport << "\\multicolumn{2}{c}{ $G_{2}= C_{2} / D_{12}$} \\\\" 
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
