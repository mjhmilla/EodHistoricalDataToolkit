//SPDX-FileCopyrightText: 2023 Matthew Millard millard.matthew@gmail.com
//SPDX-License-Identifier: MIT

#ifndef DATA_STRUCTURES
#define DATA_STRUCTURES


const char *GEN = "General";
const char *EARN = "Earnings";
const char *HIST = "History";
const char *TECH= "Technicals";
const char *FIN = "Financials";
const char *BAL = "Balance_Sheet";
const char *CF  = "Cash_Flow";
const char *IS  = "Income_Statement";
const char *OS  = "outstandingShares";

const char *Y = "yearly";
const char *A = "annual"; //EOD uses annual in the outstandingShares list.
const char *ANNUAL = "Annual";
const char *Q = "quarterly";

const double Percentiles[5]     ={0.05, 0.25, 0.5, 0.75, 0.95};

enum PercentileIndices{
  P05=0,
  P25,
  P50,
  P75,
  P95,
  NUM_PERCENTILES
};

enum EmpiricalGrowthModelTypes{
  ExponentialModel=0,
  ExponentialCyclicalModel,
  LinearModel,
  LinearCyclicalModel,
  CyclicalModel,
  NUM_EMPIRICAL_GROWTH_MODELS
};




class DataStructures {

  public:


    //============================================================================
    struct AnalysisDates{
      std::vector< std::string > common;
      std::vector< std::string > financial;
      std::vector< std::string > earningsHistory;
      std::vector< std::string > outstandingShares;
      std::vector< std::string > historical;
      std::vector< std::string > bond;
      double durationInYears;
      std::vector< unsigned int > indicesFinancial;
      std::vector< unsigned int > indicesEarningsHistory;
      std::vector< unsigned int > indicesOutstandingShares;
      std::vector< unsigned int > indicesHistorical;
      std::vector< unsigned int > indicesBond;

      std::vector< bool > isAnnualReport;
    };

    //==========================================================================
    //struct TickerMetricData{
    //  std::vector< date::sys_days > dates;
    //  std::string ticker;
    //  std::vector< std::vector< double > > metrics;
    //};

    //==========================================================================
    
    //struct MetricTable{
    //  date::sys_days dateStart;
    //  date::sys_days dateEnd;
    //  std::vector< std::string > tickers;
    //  std::vector< std::vector< double > > metrics;
    //  std::vector< std::vector< size_t > > metricRank;
    //};


    //==========================================================================    
    //
    // If this enum is changed note that there are 
    // switch statements in this header file that 
    // assume the following order
    //
    // 0. ExponentialModel
    // 1. ExponentialCyclicalModel
    // 2. LinearModel
    // 3. LinearCyclicalModel
    // 4. CyclicalModel
    //

    //==========================================================================
    //struct EmpiricalGrowthDataSetSample{
    //  std::vector< double > years;
    //  std::vector< double > afterTaxOperatingIncome;
    //};
    //==========================================================================
    
    struct EmpiricalGrowthModel{
      int modelType;
      double duration;
      double annualGrowthRateOfTrendline;
      double r2;
      double r2Trendline;
      double r2Cyclic;            
      bool validFitting;
      int outlierCount;
      std::vector< double > parameters;  
      std::vector< double > x;
      std::vector< double > y;
      std::vector< double > yTrendline;
      std::vector< double > yCyclic;
      std::vector< double > yCyclicData;
      std::vector< double > yCyclicNorm;
      std::vector< double > yCyclicNormData;
      std::vector< double > yCyclicNormDataPercentiles;
      EmpiricalGrowthModel():
        modelType(-1),
        duration(std::numeric_limits<double>::signaling_NaN()),
        annualGrowthRateOfTrendline(std::numeric_limits<double>::signaling_NaN()),
        r2(std::numeric_limits<double>::signaling_NaN()),
        r2Trendline(std::numeric_limits<double>::signaling_NaN()),
        r2Cyclic(std::numeric_limits<double>::signaling_NaN()),
        validFitting(false),
        outlierCount(0){};
    };    
    //==========================================================================
    struct EmpiricalGrowthDataSet{
      std::vector< std::string > dates;
      std::vector< double > datesNumerical;
      std::vector< double > afterTaxOperatingIncomeGrowth;
      std::vector< double > reinvestmentRate;
      std::vector< double > reinvestmentRateSD;
      std::vector< double > returnOnInvestedCapital;
      std::vector< EmpiricalGrowthModel > model;
    };

    //==========================================================================
    struct EmpiricalGrowthSettings{
      int maxDateErrorInDays;
      double growthIntervalInYears;
      double maxOutlierProportionInEmpiricalModel;
      double minCycleDurationInYears;
      double exponentialModelR2Preference;
      bool calcOneGrowthRateForAllData;
      bool includeTimeUnitInAddress;
      int typeOfEmpiricalModel;
    };

    //==========================================================================
    struct MetricGrowthDataSet{
      std::vector< std::string > dates;
      std::vector< double > datesNumerical;
      std::vector< double > metricValue;
      std::vector< double > metricGrowthRate;
      std::vector< EmpiricalGrowthModel > model;
    };

    //==========================================================================
    struct SummaryStatistics{
      std::vector< double > percentiles;
      double min;
      double max;
      double median;
      double mean;
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
    struct FinancialRatios{
      std::vector< std::string > dates;
      std::vector< double > datesNumerical;
      std::vector< double > adjustedClosePrice;
      std::vector< double > outstandingShares;
      std::vector< double > marketCapitalization;
      std::vector< double > dividendYield;
      std::vector< double > eps;
      std::vector< double > pe;
      std::vector< double > epsGaap;
      std::vector< double > peGaap;
      std::vector< double > operationalLeverage;
      std::vector< double > freeCashFlowLeverage;
      std::vector< double > earningsLeverage;
      
    };
    //==========================================================================
    struct DividendInfo{
      std::vector< std::string > dates;
      std::vector< double > datesNumerical;
      std::vector< double > dividendsPaid;
      std::vector< double > stockPrice;
      std::vector< double > dividendYield;
      std::vector< double > freeCashFlow;
      std::vector< double > freeCashFlowLessDividends;
      int yearsWithADividend;      
      double fractionOfYearsWithDividendIncreases;
      double fractionOfYearsWithCancelledDividends;      
    };


    //==========================================================================
    /**
      Many financial calculations depend on knowing the debt of a company. EOD
      reports many different types of debt though not of them are populated for
      all companies. For clairity, I'm reporting the definitions for each of 
      these fields below using EOD's glossary

      https://eodhd.com/financial-academy/financial-faq/fundamentals-glossary-common-stock

      @param shortTermDebt:
           Short-term debt refers to any debt that is due within one year or 
           less. This may include loans, credit card balances, and other 
           obligations that are expected to be paid off within the next 
           12 months. For some companies, this number represents the current 
           capital lease obligations. 
      @param shortLongTermDebt;
           the Short/Current Long-Term Debt (current portion of the long term 
           debt).           
           https://www.investopedia.com/ask/answers/06/currentportionlongtermdebt.asp
      @param shortLongTermDebtTotal;
           may differ between companies: 
            longTermDebt/longTermDebtTotal 
            +  shortTermDebt / + shortLongTermDebt 
            / = shortLongTermDebtTotal. 
      @param longTermDebt:
          any debt that is due in more than one year.
      @param longTermDebtTotal: 
          may be same as longTermDebt for some companies, 
          or longTermDebt + capitalLeaseObligations for others.
      @param capitalLeaseObligations
           long-term (non-current) amount of hire charges or rent owed by the 
           lessee to the lessor for taking capital assets on hire under a 
           capital lease. 
      @param netDebt;
            a metric that shows how much debt a company has on its balance sheet 
            compared to its liquid assets. 
            netDebt = shortTermDebt + longTermDebtTotal – cash.
      @param cash;
            the fields cash and cashAndEquivalents are interchangeable for US 
            tickers, the latter being a later addition to the template. For 
            other exchanges, this may differ in some cases, and “cash” will 
            only have the cash figure   
      @param shortTermDebtEstimate   
            This field contains shortTermDebt, or if the shortTermDebt field is 
            nan, then this field contains the estimated value, evaluated using, 
            for example, shortTermDebt = shortLongTermDebt-longTermDebt
      @param longTermDebtEstimate   
            This field contains longTermDebt, or if the longTermDebt field is 
            nan, then this field contains the estimated value, evaluated using, 
            for example, longTermDebt = shortLongTermDebt-shortTermDebt, or
            longTermDebt = netDebt-(shortTermDebt-cash)
      @param totalDebtEstimate
            This field contains totalDebt, or if the totalDebt field is 
            nan, then this field contains the estimated value, evaluated using, 
            for example, totalDebt=shortTermDebt+longTermDebt, or
            totalDebt = netDebt+cash, as netDebt=shortTermDebt+longTermDebt-cash
     */
    struct DebtInfo{
      double shortTermDebt;
      double shortLongTermDebt;
      double shortLongTermDebtTotal;
      double longTermDebt;
      double longTermDebtTotal;
      double capitalLeaseObligations;
      double netDebt;
      double cash;
      double shortTermDebtEstimate;
      std::string shortTermDebtInfo;
      double longTermDebtEstimate;
      std::string longTermDebtInfo;
      double totalDebtEstimate;
      std::string totalDebtInfo;            
      int id;
      std::string info;
      DebtInfo():
        shortTermDebt(std::nan("1")),
        shortLongTermDebt(std::nan("1")),
        shortLongTermDebtTotal(std::nan("1")),
        longTermDebt(std::nan("1")),
        longTermDebtTotal(std::nan("1")),
        capitalLeaseObligations(std::nan("1")),
        netDebt(std::nan("1")),
        cash(std::nan("1")),
        shortTermDebtEstimate(std::nan("1")),
        longTermDebtEstimate(std::nan("1")),
        totalDebtEstimate(std::nan("1")){};
    };

};



#endif