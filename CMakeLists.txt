cmake_minimum_required(VERSION 3.5)
include(GenerateExportHeader)
include(ExternalProject)
project(wamp2)

find_package(websocketpp QUIET)
if(websocketpp_FOUND)
else()
    ExternalProject_Add(
      websocketpp
      GIT_REPOSITORY "https://github.com/zaphoyd/websocketpp.git"
      GIT_TAG "master"
      PREFIX websocketpp
      UPDATE_COMMAND ""
      PATCH_COMMAND ""
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=install
      TEST_COMMAND ""
    )
    set(WEBSOCKETPP_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/websocketpp/src/websocketpp-build/install/include ${PROJECT_BINARY_DIR}/websocketpp/src/websocketpp/install/include)
    message(STATUS "Websocketpp: " ${WEBSOCKETPP_INCLUDE_DIRS})
    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        set(WEBSOCKETPP_LIBRARIES ${WEBSOCKETPP_LIBRARIES} ws2_32)
    endif()
endif()
add_definitions(${WEBSOCKETPP_DEFINITIONS})


ExternalProject_Add(
  asio
  GIT_REPOSITORY "https://github.com/chriskohlhoff/asio.git"
  GIT_TAG "master"
  PREFIX asio
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  UPDATE_COMMAND ""
  PATCH_COMMAND ""
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=install
  TEST_COMMAND ""
  INSTALL_COMMAND ""
)
set(ASIO_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/asio/src/asio/asio/include)


ExternalProject_Add(
      msgpack
      GIT_REPOSITORY "https://github.com/msgpack/msgpack-c.git"
      GIT_TAG "master"
      PREFIX msgpack
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      UPDATE_COMMAND ""
      PATCH_COMMAND ""
      TEST_COMMAND ""
    )
set(MSGPACK_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/msgpack/src/msgpack/include)
message(STATUS "MsgPack: " ${MSGPACK_INCLUDE_DIRS})

ExternalProject_Add(
      cryptopp
      GIT_REPOSITORY "https://github.com/weidai11/cryptopp.git"
      GIT_TAG "master"
      PREFIX cryptopp
      UPDATE_COMMAND ""
      PATCH_COMMAND ""
      TEST_COMMAND ""
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=install
    )
set(CRYPTOPP_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/cryptopp/src/cryptopp-build/install/include)
set(CRYPTOPP_LIB_DIRS ${PROJECT_BINARY_DIR}/cryptopp/src/cryptopp-build/install/lib)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CRYPTOPP_LIBRARY ${CRYPTOPP_LIB_DIRS}/${CMAKE_STATIC_LIBRARY_PREFIX}cryptopp${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CRYPTOPP_LIBRARY ${CRYPTOPP_LIB_DIRS}/${CMAKE_STATIC_LIBRARY_PREFIX}cryptopp-static${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

ExternalProject_Add(
      json
      GIT_REPOSITORY "https://github.com/nlohmann/json.git"
      GIT_TAG "master"
      PREFIX json
      UPDATE_COMMAND ""
      PATCH_COMMAND ""
      TEST_COMMAND ""
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=install
    )
set(JSON_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/json/src/json-build/install/include)

enable_testing()
add_subdirectory(test)
set(EXECUTORS_BUILD_TESTS OFF)
add_subdirectory(executors)
file(GLOB DATABASE_SRCS
         RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_library(${PROJECT_NAME} STATIC ${DATABASE_SRCS})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
link_directories(${CRYPTOPP_LIB_DIRS})
target_link_libraries(${PROJECT_NAME} executors ${WEBSOCKETPP_LIBRARIES} ${CRYPTOPP_LIBRARY})
add_dependencies(${PROJECT_NAME} executors websocketpp asio msgpack cryptopp json)
generate_export_header(${PROJECT_NAME})
include_directories(${WEBSOCKETPP_INCLUDE_DIRS} ${ASIO_INCLUDE_DIRS}
    ${MSGPACK_INCLUDE_DIRS} ${CRYPTOPP_INCLUDE_DIRS} ${JSON_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR} ${WEBSOCKETPP_INCLUDE_DIRS} ${ASIO_INCLUDE_DIRS}
    ${MSGPACK_INCLUDE_DIRS} ${CRYPTOPP_INCLUDE_DIRS} ${JSON_INCLUDE_DIRS})


