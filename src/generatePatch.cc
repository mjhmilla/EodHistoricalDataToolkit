
#include <cstdio>
#include <fstream>
#include <string>
#include <regex>

#include <nlohmann/json.hpp>
#include <tclap/CmdLine.h>

#include <filesystem>

#include "FinancialAnalysisToolkit.h"

#include <boost/algorithm/string/classification.hpp> // Include boost::for is_any_of
#include <boost/algorithm/string/split.hpp> // Include for boost::split
#include <boost/algorithm/string.hpp> // Include for boost::iequals

unsigned int COLUMN_WIDTH = 30;

struct primaryTickerPatchData {
  std::string currency;
  std::vector< std::string > exchanges;
  std::vector< std::string > tickersInError;
  std::vector< bool > patchFound;
};


void trim(std::string& str,
          const std::string& whitespace)
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin != std::string::npos){
      const auto strEnd = str.find_last_not_of(whitespace);
      const auto strRange = strEnd - strBegin + 1;
      str = str.substr(strBegin, strRange);
    }

}
// Function to find Levenshtein distance between string `X` and `Y`.
// `m` and `n` is the total number of characters in `X` and `Y`, respectively
// This source code comes from:
//    https://www.techiedelight.com/levenshtein-distance-edit-distance-problem/
// which was described in
//    https://en.wikipedia.org/wiki/Levenshtein_distance
unsigned int calcLevenshteinDistance(std::string& x, std::string& y)
{
    unsigned int m = x.length();
    unsigned int n = y.length();  

    // For all pairs of `i` and `j`, `T[i, j]` will hold the Levenshtein distance
    // between the first `i` characters of `X` and the first `j` characters of `Y`.
    // Note that `T` holds `(m+1)×(n+1)` values.
    int T[m + 1][n + 1];
 
    // initialize `T` by all 0's
    std::memset(T, 0, sizeof(T));
 
    // we can transform source prefixes into an empty string by
    // dropping all characters 
    for (int i = 1; i <= m; ++i){
        T[i][0] = i;                // (case 1)
    }
 
    // we can reach target prefixes from empty source prefix
    // by inserting every character 
    for (int j = 1; j <= n; ++j){
        T[0][j] = j;                // (case 1)
    }
 
    int substitutionCost;
 
    // fill the lookup table in a bottom-up manner
    for (int i = 1; i <= m; ++i){
        for (int j = 1; j <= n; ++j){
            if (x[i - 1] == y[j - 1]) {                 // (case 2)
                substitutionCost = 0;                   // (case 2)
            } else{
                substitutionCost = 1;                   // (case 3c)
            }
            T[i][j] = std::min(std::min(T[i - 1][j] + 1,      // deletion (case 3b)
                                        T[i][j - 1] + 1),     // insertion (case 3a)
                      T[i - 1][j - 1] + substitutionCost);    // replace (case 2 & 3c)
        }
    }
    return T[m][n];
}

int main (int argc, char* argv[]) {

  std::string exchangeCode;
  std::string scanFileName;
  std::string exchangeListFileName;
  std::string exchangeSymbolListFolder;
  std::string outputFolder;
  bool verbose;

  try{
    TCLAP::CmdLine cmd("The command will read the output from the scan "
    "function and will try to find patches for the problems identified "
    "by scan. For example, a company that is on a German "
    "exchange, does its accounting in USD, yet does not have a "
    "PrimaryTicker entry is probably in error: this company does not "
    "reside in Germany if it does its accounting in USD and so it must "
    "have a missing PrimaryTicker. Information from the exchange list, "
    "and exchange-symbol-lists is searched to find a company that resides"
    "on an exchange with the same currency (USD in this example) and has "
    "a very similar company name."
    ,' ', "0.0");




    TCLAP::ValueArg<std::string> scanFileNameInput("c",
      "scan_file_name", 
      "The full path and file name for the file produced by the scan "
      "function (e.g. STU.scan.json).",
      true,"","string");

    cmd.add(scanFileNameInput);

    TCLAP::ValueArg<std::string> exchangeCodeInput("x","EXCHANGE_CODE", 
      "The exchange code. For example: US",
      true,"","string");

    cmd.add(exchangeCodeInput);  


    TCLAP::ValueArg<std::string> exchangeSymbolListFolderInput("s",
      "exchange_symbol_list", "A folder that contains json files, one per "
      "exchange, that lists the tickers in each exchange",
      true,"","string");


    cmd.add(exchangeSymbolListFolderInput);  

    TCLAP::ValueArg<std::string> exchangeListFileNameInput("l","exchange_list", 
      "The exchanges-list json file generated by EOD",
      true,"","string");

    cmd.add(exchangeListFileNameInput); 

    TCLAP::ValueArg<std::string> outputFolderInput("o","output_folder_path", 
      "The path to the folder that will contain the output json files "
      "produced by this analysis",
      true,"","string");

    cmd.add(outputFolderInput);

    TCLAP::SwitchArg verboseInput("v","verbose",
      "Verbose output printed to screen", false);
    cmd.add(verboseInput);    

    cmd.parse(argc,argv);

    scanFileName              = scanFileNameInput.getValue();
    exchangeCode              = exchangeCodeInput.getValue();
    exchangeListFileName      = exchangeListFileNameInput.getValue(); 
    exchangeSymbolListFolder  = exchangeSymbolListFolderInput.getValue();   
    outputFolder              = outputFolderInput.getValue();
    verbose                   = verboseInput.getValue();

    if(verbose){
      std::cout << "  Scan File Name" << std::endl;
      std::cout << "    " << scanFileName << std::endl;
      
      std::cout << "  Exchange Code" << std::endl;
      std::cout << "    " << exchangeCode << std::endl;

      std::cout << "  Exchange List File" << std::endl;
      std::cout << "    " << exchangeListFileName << std::endl;

      std::cout << "  Exchange Symbol List" << std::endl;
      std::cout << "    " << exchangeSymbolListFolder << std::endl;

      std::cout << "  Output Folder" << std::endl;
      std::cout << "    " << outputFolder << std::endl;
    }
  } catch (TCLAP::ArgException &e)  // catch exceptions
	{ 
    std::cerr << "error: "    << e.error() 
              << " for arg "  << e.argId() << std::endl; 
  }


  std::string validFileExtension = exchangeCode;
  validFileExtension.append(".json");



  unsigned int count=0;
  unsigned int errorCount=0;

  using json = nlohmann::ordered_json;
  std::ifstream scanFileStream(scanFileName.c_str());
  json scanResults = json::parse(scanFileStream);

  json patchResults;

  //Scan the list of exhanges for potential patches
  if(verbose){
    std::cout << std::endl;
    std::cout << "Scanning exchange-symbol-lists for companies " << std::endl;
    std::cout << "with the same name. These are stored as a" << std::endl;
    std::cout << "potential PrimaryTickerPatch" << std::endl;
  }

  //load the exchange-list
  std::ifstream exchangeListFileStream(exchangeListFileName.c_str());
  json exchangeList = json::parse(exchangeListFileStream);

  //scan the errors and group them by currency
  std::vector< primaryTickerPatchData > primaryTickerPatchDataVector;

  unsigned int patchCount = 0;


  //1. Sort MissingPrimaryTicker errors by currency.
  //2. Identify the exchanges that use that currency
  //3. Then use this list to look for matching company names so that you 
  //   only have to open each exchange symbol list once.
  for(auto& i : scanResults){

    if(i["MissingPrimaryTicker"].get<bool>()){

      json patchEntry = 
        json::object( 
            { 
              {"Code", i["Code"].get<std::string>()},
              {"Name", i["Name"].get<std::string>()},
              {"Exchange", i["Exchange"].get<std::string>()},
              {"PrimaryTicker", i["PrimaryTicker"].get<std::string>()},
              {"PatchFound",false},
              {"PatchPrimaryTicker", ""},
              {"PatchName",""},
              {"PatchNameSimilarityScore",0.},
              {"PatchNameExactMatch",false},
              {"PatchNameFirstWordsMatch",false},
              {"PatchNameLengthAcceptable",true},
              {"PatchSecondCandidateName",""},
              {"PatchSecondCandidateCode",""},
              {"PatchSecondCandidateScore",0},
            }
          );

      patchResults[i["Code"].get<std::string>()]= patchEntry;

      std::string currency_symbol;
      FinancialAnalysisToolkit::
        getJsonString(i["currency_symbol"],currency_symbol);

      bool appendNewPatchData=true;
      for(auto& j : primaryTickerPatchDataVector){
        //If the currency exists, then add to the existing patch
        if( j.currency.compare(currency_symbol)==0){
          appendNewPatchData=false;
          std::string code;
          FinancialAnalysisToolkit::getJsonString(i["Code"],code);            
          j.tickersInError.push_back(code);
          j.patchFound.push_back(false);
        }
      }

      //If the currency doesn't exist, then make a new entry
      if(appendNewPatchData){
        primaryTickerPatchData patch;
        patch.currency = currency_symbol;

        std::string code;
        FinancialAnalysisToolkit::getJsonString(i["Code"],code);            
        patch.tickersInError.push_back(code);
        patch.patchFound.push_back(false);

        //Make a list that contains every exchange that uses this currency
        for(auto& itExc : exchangeList){
          std::string exchangeCurrency;
          FinancialAnalysisToolkit:: 
            getJsonString(itExc["Currency"],exchangeCurrency);
            if(patch.currency.compare(exchangeCurrency)==0){
              std::string exchangeCode;
              FinancialAnalysisToolkit::getJsonString(itExc["Code"],exchangeCode);
              patch.exchanges.push_back(exchangeCode);
            }
        }
        primaryTickerPatchDataVector.push_back(patch);
      }
    }
  }

  //The data in primaryTickerPatchDataVector has been organized so that
  //the file for a specific exchange only needs to be opened once to 
  //reduce the run time.
  for (auto& itPatchData : primaryTickerPatchDataVector){
    for (auto& itExc : itPatchData.exchanges ){
      //load the exchange-symbol-list
      std::string exchangeSymbolListPath = exchangeSymbolListFolder;
      exchangeSymbolListPath.append(itExc);
      exchangeSymbolListPath.append(".json");
      std::ifstream exchangeSymbolListFileStream(exchangeSymbolListPath);
      json exchangeSymbolList = json::parse(exchangeSymbolListFileStream);        

      //for(auto& indexTicker : itPatchData.tickersInError){
      for(unsigned int indexTicker=0; 
            indexTicker<itPatchData.tickersInError.size();  ++indexTicker){

        //Get the company name
        std::string nameARaw,nameA;
        std::string tickerName = itPatchData.tickersInError[indexTicker];
        FinancialAnalysisToolkit::
          getJsonString(scanResults[tickerName]["Name"],nameARaw);
        nameA=nameARaw;

        //Remove extra additions that are not a part of any company name
        //Remove text between pairs of ( ) and - -
        nameA = std::regex_replace(nameA, 
                                                std::regex("-.*-.*\\(.*\\)"), "");
        //Remove non-alphanumeric characters                                                
        nameA = std::regex_replace(nameA, 
                                                std::regex("[^a-zA-Z\\d\\s:]"), "");
        //Trim leading and following whitespace
        trim(nameA," ");

        //Break the company name into an std::vector of words
        std::vector<std::string> wordsA;
        boost::split(wordsA, nameA, boost::is_any_of(" "), 
                    boost::token_compress_on);

        //Convert it all to lower case
        std::transform(wordsA.begin(), wordsA.end(), wordsA.begin(),
            [](std::string &stringA)
            {
                std::transform(stringA.begin(), stringA.end(), stringA.begin(),
                    [](unsigned char c) { return std::tolower(c); });
                return stringA;
            });

        //Get the length of the sum of strings in wordsA
        unsigned int wordsALength = 
            std::accumulate(wordsA.begin(), wordsA.end(), 0,
                            [](unsigned int sum, const std::string& elem) {
                                return sum + elem.size();
                            });

        //Calculate the minimum acceptable score
        double minMatchingWordFraction=0.5;                            

        unsigned int maxLength = wordsALength + (wordsA.size()-1);
        maxLength = maxLength*2;

        unsigned int minScore = 
          (double)(minMatchingWordFraction*((double)wordsALength));

        if(minScore < 3){
          minScore = 3;
        }

        bool candidateFound = false;
        unsigned int score  = 0;
        double bestScore    = patchResults[tickerName]["PatchNameSimilarityScore"];

        std::string bestName;
        std::string bestCode;
        std::string nameB,nameBRaw;

        unsigned int smallWord = 3;

        for(auto& itSym : exchangeSymbolList){
          bool exactMatch=false;
          FinancialAnalysisToolkit::getJsonString(itSym["Name"],nameBRaw);
          nameB=nameBRaw;
          //convert to lower case
          std::transform(nameB.begin(),nameB.end(),nameB.begin(),
            [](unsigned char c){return std::tolower(c);});

          bool firstWordsMatch = false;
          bool lengthAcceptable= (nameB.length() < maxLength) ? true:false;

          //Evaluate the similarity score
          if(nameARaw.compare(nameBRaw)==0){
            score = nameA.size();
            exactMatch=true;
          }else{
            if(nameARaw.compare("Beyond Meat Inc")==0 &&
               nameBRaw.find("Beyond Minerals Inc") != std::string::npos ){
                bool here=true;
               }

            score=0;
            size_t pos =0;
            size_t loc = 0;
            unsigned int lengthOfMatchingWords=0;            

            for(unsigned int i=0; i < wordsA.size();++i){

              loc = nameB.find(wordsA[i],pos);

              if(loc != std::string::npos){

                char prev = ' ';
                if(i>0){
                  prev = nameB[loc-1];
                }
                char next = ' ';
                if(loc+wordsA[i].length()<nameB.length()){
                    next = nameB[loc+wordsA[i].length()];
                }

                bool wordValid = false;
                if( !std::isalpha(prev) && !std::isalpha(next)){
                  wordValid=true;
                }

                if(wordValid){
                  pos += loc + wordsA[i].length();
                  score+=wordsA[i].length();
                  if(i==0 && loc == 0){
                    firstWordsMatch=true;
                  }
                }
              }
            }
          }



          if((score > bestScore && score > minScore 
              && firstWordsMatch && lengthAcceptable) || exactMatch){// && score > minScoreA && score > minScoreB ){
            bestScore=score;
            candidateFound=true;
            FinancialAnalysisToolkit::
              getJsonString(itSym["Name"],bestName);            
            FinancialAnalysisToolkit::
              getJsonString(itSym["Code"],bestCode);
              bestCode.append(".");
              bestCode.append(itExc);
              patchResults[tickerName]["PatchFound"] = true;
              patchResults[tickerName]["PatchPrimaryTicker"] = bestCode;
              patchResults[tickerName]["PatchName"]=bestName;
              patchResults[tickerName]["PatchNameSimilarityScore"] = bestScore;
              patchResults[tickerName]["PatchNameFirstWordsMatch"]=true;
              patchResults[tickerName]["PatchNameLengthAcceptable"]=true;

              itPatchData.patchFound[indexTicker]=true;

            if(exactMatch){
              patchResults[tickerName]["PatchNameExactMatch"]=true;
              break;
            }
          }else if( score > 
            patchResults[tickerName]["PatchSecondCandidateScore"].get<unsigned int>()){
            std::string secondCode,secondName;
            FinancialAnalysisToolkit::
              getJsonString(itSym["Name"],secondName);            
            FinancialAnalysisToolkit::
              getJsonString(itSym["Code"],secondCode);            
            secondCode.append(".");
            secondCode.append(itExc);


            patchResults[tickerName]["PatchSecondCandidateName"]=secondName;
            patchResults[tickerName]["PatchSecondCandidateCode"]=secondCode;
            patchResults[tickerName]["PatchSecondCandidateScore"]=score;
          } 
        }

      }

    }
  }
  //Report on the items that were patched and not.
  if(verbose){
    std::cout << std::endl;
    
    std::cout << "Patches found for : " << std::endl;
    unsigned int  countPatches = 0;
    for(auto& itData : primaryTickerPatchDataVector){
      for(unsigned int i=0; i<itData.patchFound.size();++i){
        if(itData.patchFound[i]==true){
          std::string nameA("");
          FinancialAnalysisToolkit::
            getJsonString(scanResults[itData.tickersInError[i]]["Name"],
                          nameA);
          ++countPatches;
          std::cout << countPatches 
              << ".  " << itData.tickersInError[i] << std::endl
              << "  " << patchResults[itData.tickersInError[i]]["PatchNameSimilarityScore"].get<unsigned int>() 
              << "/" << nameA.size() << std::endl
              << "  " << nameA << std::endl
              << "  " << patchResults[itData.tickersInError[i]]
                                     ["PatchName"].get<std::string>() 
              << std::endl;
        }
      }
    }
    std::cout << std::endl << std::endl;
    std::cout << "Patches could not be found for : " << std::endl;
    unsigned int  countMissingPatches = 0;
    for(auto& itData : primaryTickerPatchDataVector){
      for(unsigned int i=0; i<itData.patchFound.size();++i){
        if(itData.patchFound[i]==false){
          std::string nameA("");
          FinancialAnalysisToolkit::
            getJsonString(scanResults[itData.tickersInError[i]]["Name"],
                          nameA);
          ++countMissingPatches;
          std::cout << countMissingPatches 
              << ".  " << itData.tickersInError[i] << std::endl
              << "  " << nameA  << std::endl;
          std::cout << "  " << itData.currency << ": ";
          for(unsigned int j=0; j<itData.exchanges.size();++j){
            std::cout << itData.exchanges[j] << " " ;
          }
          std::cout << std::endl;
        }
      }
    }
  }
    



  std::string outputFilePath(outputFolder);
  std::string outputFileName(exchangeCode);
  outputFileName.append(".patch");
  outputFileName.append(".json");
  
  //Update the extension 
  outputFilePath.append(outputFileName);

  std::ofstream outputFileStream(outputFilePath,
      std::ios_base::trunc | std::ios_base::out);
  outputFileStream << patchResults;
  outputFileStream.close();

  std::cout << "success" << std::endl;

  return 0;
}
