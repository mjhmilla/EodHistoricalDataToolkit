
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

struct primaryTickerPatchData {
  std::string currency;
  std::vector< std::string > exchanges;
  std::vector< std::string > tickersInError;
  std::vector< bool > patchFound;
};

struct TextSimilarity{
  double score=0;
  bool firstWordsMatch;
  bool allWordsFound;
  bool exactMatch;
};

struct WordData{
  std::string raw;
  std::string lowercase;
  std::vector<std::string> words;
  double alphaLength;
  WordData(std::string& str){
    raw=str;
    lowercase=str;
    std::transform( lowercase.begin(),
                lowercase.end(),
                lowercase.begin(),
                [](unsigned char c){return std::tolower(c);});

    //Remove extra additions that are not a part of any company name
    //Remove text between pairs of ( ) and - -
    lowercase = std::regex_replace(lowercase, 
                            std::regex("-.*-.*\\(.*\\)"), "");
    //Remove non-alphanumeric characters                                                
    lowercase = std::regex_replace(lowercase, 
                            std::regex("[^a-zA-Z\\d\\s:]"), "");
    //Trim leading and following whitespace
    trim(lowercase," ");        

    //Break the company name into an std::vector of words
    boost::split(words, lowercase, boost::is_any_of(" "), 
                boost::token_compress_on);
    alphaLength= std::count_if(
                    lowercase.begin(),
                    lowercase.end(),
                    [](unsigned char c){
                        return std::isalpha(c);});

  };
};



struct TextData{
  std::string raw;
  std::string lowercase;
  std::vector<std::string> words;
  double alphaLength;
  TextData(std::string& str){
    raw=str;
    lowercase=str;
    std::transform( lowercase.begin(),
                lowercase.end(),
                lowercase.begin(),
                [](unsigned char c){return std::tolower(c);});
    alphaLength= std::count_if(
                    lowercase.begin(),
                    lowercase.end(),
                    [](unsigned char c){
                        return std::isalpha(c);});

  }
};

void evaluateSimilarity(WordData &textA,
                        TextData &textB,
                        TextSimilarity &result){

  double scoreA=0.;
  double scoreB=0.;                          

  //Evaluate the similarity score
  if(textA.raw.compare(textB.raw)==0){
    scoreA = 1.0;
    scoreB = 1.0;
    result.exactMatch=true;
    
  }else if(textB.raw.find(textA.raw) != std::string::npos){
    result.allWordsFound = true;
    scoreA = ( textA.alphaLength / textA.alphaLength );
    scoreB = ( textA.alphaLength / textB.alphaLength );  

  }else if(textA.raw.find(textB.raw) != std::string::npos){
    result.allWordsFound = true;
    scoreA = ( textB.alphaLength / textA.alphaLength );
    scoreB = ( textB.alphaLength / textB.alphaLength );  

  }else{

    size_t pos =0;
    size_t loc = 0;
    unsigned int lengthOfMatchingWords=0;            
    result.allWordsFound = true;

    for(unsigned int i=0; i < textA.words.size();++i){
      pos = 0;
      bool wordValid = false;
      while(loc != std::string::npos && wordValid==false){

        loc = textB.lowercase.find(textA.words[i],pos);                
        pos += loc + textA.words[i].length();

        if(loc != std::string::npos){
          char prev = ' ';
          if(i>0){
            prev = textB.lowercase[loc-1];
          }
          char next = ' ';
          if(loc+textA.words[i].length()<textB.lowercase.length()){
              next = textB.lowercase[loc+textA.words[i].length()];
          }                  
          if( !std::isalpha(prev) && !std::isalpha(next)){
            wordValid=true;
          }
        }
      }
      if(!wordValid){
        result.allWordsFound=false;
      }
      if(wordValid){                
        double wordLength = textA.words[i].length();
        scoreA+= ( wordLength / textA.alphaLength );
        scoreB+= ( wordLength / textB.alphaLength );
        if(i==0 && loc == 0){
          result.firstWordsMatch=true;
        }
      }              
    }
    
  }

  result.score = std::min(scoreA,scoreB);
  
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
              {"PatchNameClosest",""},
              {"PatchCodeClosest",""},
              {"PatchNameSimilarityScoreClosest",0},
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

  unsigned int numElementsToProcess=0;
  for(auto& j : primaryTickerPatchDataVector){
    numElementsToProcess += j.tickersInError.size()*j.exchanges.size();
  }


  //The data in primaryTickerPatchDataVector has been organized so that
  //the file for a specific exchange only needs to be opened once to 
  //reduce the run time.
  if(verbose){
    std::cout << "Processing "  << std::endl;
  }
  unsigned int tickerCount = 0;
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
        ++tickerCount;
        if(verbose){
          std::string code=itPatchData.tickersInError[indexTicker];
          std::cout << tickerCount << '\t' <<"/" << numElementsToProcess << std::endl;
          std::cout << '\t' << scanResults[code]["Code"].get<std::string>() 
                    << std::endl;
          std::cout << '\t' << scanResults[code]["Name"].get<std::string>() 
                    << std::endl;
          std::cout << std::endl;
        }        

        std::string nameARaw;
        std::string tickerName = itPatchData.tickersInError[indexTicker];
        FinancialAnalysisToolkit::
          getJsonString(scanResults[tickerName]["Name"],nameARaw);

        WordData textA(nameARaw);


        //Calculate the minimum acceptable score
        double minMatchingWordFraction=0.3;                            

        bool candidateFound = false;
        double bestScore    = 
          patchResults[tickerName]["PatchNameSimilarityScore"].get<double>();

        std::string bestName;
        std::string bestCode;

        for(auto& itSym : exchangeSymbolList){

  
          std::string nameBRaw;
          FinancialAnalysisToolkit::getJsonString(itSym["Name"],nameBRaw);
          TextData textB(nameBRaw);

          if(nameARaw.find("Osisko") != std::string::npos &&
              nameBRaw.find("Osisko") != std::string::npos ){
              bool here=true;
              }

          TextSimilarity simAB;

          simAB.score=0;
          simAB.firstWordsMatch=false;
          simAB.allWordsFound=false;
          simAB.exactMatch=false;

          evaluateSimilarity(textA,textB,simAB);



          if( ( simAB.score > bestScore &&
                simAB.score > minMatchingWordFraction)  ||
              ( simAB.exactMatch || simAB.allWordsFound)) {

            bestScore=simAB.score;
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

              itPatchData.patchFound[indexTicker]=true;

            if(simAB.exactMatch){
              patchResults[tickerName]["PatchNameExactMatch"]=true;
              break;
            }
          }else if( simAB.score > 
            patchResults[tickerName]["PatchNameSimilarityScoreClosest"].get<double>()){
            std::string secondCode,secondName;
            FinancialAnalysisToolkit::
              getJsonString(itSym["Name"],secondName);            
            FinancialAnalysisToolkit::
              getJsonString(itSym["Code"],secondCode);            
            secondCode.append(".");
            secondCode.append(itExc);


            patchResults[tickerName]["PatchNameClosest"]=secondName;
            patchResults[tickerName]["PatchCodeClosest"]=secondCode;
            patchResults[tickerName]["PatchNameSimilarityScoreClosest"] 
              = simAB.score;
          } 
        }

      }

    }
  }

  //Split patchResults into:
  // patch.json            : exact match
  // patch.candidate.json  : not exact, but met the conditions
  // patch.missing.json    : did not meet conditions

json patchExactList;
json patchCandidateList;
json patchMissingList;

for(auto& it : patchResults){
  bool patchFound = it["PatchFound"].get<bool>();
  bool patchExact = it["PatchNameExactMatch"].get<bool>();
  std::string code;
  FinancialAnalysisToolkit::getJsonString(it["Code"],code);        

  if(patchExact){
    json patchExactEntry = 
      json::object( 
          { 
            {"Code", it["Code"].get<std::string>()},
            {"Name", it["Name"].get<std::string>()},
            {"Exchange", it["Exchange"].get<std::string>()},
            {"PrimaryTicker", it["PatchPrimaryTicker"].get<std::string>()},
            {"PatchName",it["PatchName"]},
          }
        );
    patchExactList[code]=patchExactEntry;  
  }
   
  if(!patchExact && patchFound){
    json patchCandidateEntry = 
      json::object( 
          { 
            {"Code", it["Code"].get<std::string>()},
            {"Name", it["Name"].get<std::string>()},
            {"Exchange", it["Exchange"].get<std::string>()},
            {"PrimaryTicker", it["PatchPrimaryTicker"].get<std::string>()},
            {"PatchName",it["PatchName"]},
            {"PatchNameSimilarityScore", it["PatchNameSimilarityScore"].get<double>()},
          }
        );
    patchCandidateList[code]=patchCandidateEntry;  

  }

  if(!patchExact && !patchFound){
    json patchMissingEntry = 
      json::object( 
          { 
            {"Code", it["Code"].get<std::string>()},
            {"Name", it["Name"].get<std::string>()},
            {"Exchange", it["Exchange"].get<std::string>()},
            {"PrimaryTicker", it["PatchPrimaryTicker"].get<std::string>()},
            {"PatchNameClosest",it["PatchNameClosest"]},
            {"PatchNameSimilarityScoreClosest", 
              it["PatchNameSimilarityScoreClosest"].get<double>()},
          }
        );
    patchMissingList[code]=patchMissingEntry;  
  }



  }

  //Report on the items that were patched and not.
  if(verbose){
    std::cout << std::endl;    
    std::cout << "Patches found for : " << std::endl;
    std::cout << std::endl;

    unsigned int count = 1;
    for(auto& it : patchExactList){
      std::cout << count << "." << '\t' 
                << it["Code"].get<std::string>() << '\t' << '\t'   
                << it["Name"].get<std::string>() << std::endl
                << '\t' << it["PrimaryTicker"].get<std::string>() 
        << '\t' << '\t' << it["PatchName"].get<std::string>() << std::endl;
      std::cout << std::endl;

      ++count;
    }

    std::cout << std::endl;    
    std::cout << "Patch candidates found for : " << std::endl;
    std::cout << std::endl;

    count =1;
    for(auto& it : patchCandidateList){
      std::cout << count << "." << '\t' 
                << it["Code"].get<std::string>() << '\t' 
                << it["PrimaryTicker"].get<std::string>() << std::endl
                << '\t' << it["Name"].get<std::string>() << std::endl                
                << '\t' << it["PatchName"].get<std::string>() << std::endl;

      std::cout << '\t' << it["PatchNameSimilarityScore"].get<double>() 
                << std::endl << std::endl;
      ++count;
    }


    std::cout << std::endl;    
    std::cout << "Patch missing for : " << std::endl;
    std::cout << std::endl;

    count =1;
    for(auto& it : patchMissingList){
      std::cout << count << "." << '\t' 
                << it["Code"].get<std::string>() << std::endl  
                << '\t' << it["Name"].get<std::string>() << std::endl
                << '\t' << it["PatchNameClosest"].get<std::string>() << std::endl
                << '\t' << it["PatchNameSimilarityScoreClosest"].get<double>();                 
      std::cout << std::endl;
      std::cout << std::endl;
      ++count;
    }

  }
    


  //Output the list of exact patches
  std::string outputFilePath(outputFolder);
  std::string outputFileName(exchangeCode);
  outputFileName.append(".patch.json");
  outputFilePath.append(outputFileName);

  std::ofstream outputFileStream(outputFilePath,
      std::ios_base::trunc | std::ios_base::out);
  outputFileStream << patchExactList;
  outputFileStream.close();

  //Output the list of candidate patches
  outputFilePath = outputFolder;
  outputFileName = exchangeCode;
  outputFileName.append(".patch.candidate.json");
  outputFilePath.append(outputFileName);

  outputFileStream.open(outputFilePath,
      std::ios_base::trunc | std::ios_base::out);
  outputFileStream << patchCandidateList;
  outputFileStream.close();

  //Output the list of missing patches
  outputFilePath = outputFolder;
  outputFileName = exchangeCode;
  outputFileName.append(".patch.missing.json");
  outputFilePath.append(outputFileName);

  outputFileStream.open(outputFilePath,
      std::ios_base::trunc | std::ios_base::out);
  outputFileStream << patchMissingList;
  outputFileStream.close();


  std::cout << "success" << std::endl;

  return 0;
}
