PROJECT (SCREENER CXX)

CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

LIST( APPEND CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/CMake
  ${PROJECT_SOURCE_DIR}/CMake/Modules)

SET(CUSTOM_NLOHMANN_INCLUDE_PATH "" CACHE PATH "Path to include/json.hpp")

SET(CUSTOM_SCIPLOT_INCLUDE_PATH "" CACHE PATH "Path to sciplot/sciplot.hpp")


SET(BOOST_INC_DIR
  /usr/include
  CACHE STRING
  "Location of the BOOST header files")


FIND_PACKAGE (Eigen3 3.0.0 REQUIRED)
FIND_PACKAGE (TCLAP REQUIRED tclap>=1.2.0)
FIND_PACKAGE (CURL REQUIRED)
FIND_PACKAGE (Boost REQUIRED)

INCLUDE_DIRECTORIES(
  ${EIGEN3_INCLUDE_DIR} 
  ${TCLAP_INCLUDE_PATH}
  ${CURL_INCLUDE_DIR}
  ${BOOST_INCLUDE_DIRS}   
  ${CUSTOM_NLOHMANN_INCLUDE_PATH}
  ${CUSTOM_SCIPLOT_INCLUDE_PATH})


ADD_EXECUTABLE(
  sandbox
  src/sandbox.cc)
  
ADD_EXECUTABLE(
  fetch
  src/fetch.cc)
  
ADD_EXECUTABLE(
  scan
  src/scan.cc)
  
ADD_EXECUTABLE(
  generatePatch
  src/generatePatch.cc)  

ADD_EXECUTABLE(
  applyPatch
  src/applyPatch.cc)  

ADD_EXECUTABLE(
  calculate
  src/calculate.cc)

ADD_EXECUTABLE(
  jsonTools
  src/jsonTools.cc)

ADD_EXECUTABLE(
  csvTools
  src/csvTools.cc)

ADD_EXECUTABLE(
  generateTickerReports
  src/generateTickerReports.cc)  
  
ADD_EXECUTABLE(
  generateScreenerReport
  src/generateScreenerReport.cc)  

ADD_EXECUTABLE(
  generateComparisonReport
  src/generateComparisonReport.cc)  

TARGET_LINK_LIBRARIES(sandbox
  ${CURL_LIBRARIES}
)

TARGET_LINK_LIBRARIES(fetch
  ${CURL_LIBRARIES}
)

TARGET_LINK_LIBRARIES(applyPatch
  ${CURL_LIBRARIES}
)

TARGET_LINK_LIBRARIES(generatePatch
  ${CURL_LIBRARIES}
)

message("EIGEN3_INCLUDE_DIR           :" ${EIGEN3_INCLUDE_DIR})
message("TCLAP_INCLUDE_DIR            :" ${TCLAP_INCLUDE_PATH})
message("CURL_INCLUDE_DIR             :" ${CURL_INCLUDE_DIR})
message("BOOST_INCLUDE_DIR            :" ${Boost_INCLUDE_DIRS})
message("CUSTOM_NLOHMANN_INCLUDE_PATH :" ${CUSTOM_NLOHMANN_INCLUDE_PATH})
message("CUSTOM_SCIPLOT_INCLUDE_PATH  :" ${CUSTOM_SCIPLOT_INCLUDE_PATH})
