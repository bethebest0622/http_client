file(GLOB UTIL ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_subdirectory(../../third_party/drogon drogon_bin_src)

add_library(curl_util SHARED ${UTIL})
target_link_libraries(curl_util config++ util_cpp drogon)
# drogon trantor uuid jsoncpp ssl z dl pthread)
install(TARGETS curl_util LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

set(LOCAL_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../include)
install(DIRECTORY ${LOCAL_SRC_PATH}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include/curl_util FILES_MATCHING REGEX ".*\\.(h|hpp|hxx)")

set(LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib_hft/)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib_hft/)
  install(TARGETS curl_util LIBRARY DESTINATION ${LIB_PATH}/lib)
  install(DIRECTORY ${LOCAL_SRC_PATH}/ DESTINATION ${LIB_PATH}/include/curl_util/ FILES_MATCHING REGEX ".*\\.(h|hpp|hxx)")
endif()
