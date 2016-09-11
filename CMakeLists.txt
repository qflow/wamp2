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
target_link_libraries(${PROJECT_NAME} executors ${WEBSOCKETPP_LIBRARIES})
add_dependencies(${PROJECT_NAME} executors websocketpp asio msgpack)
generate_export_header(${PROJECT_NAME})
include_directories(${WEBSOCKETPP_INCLUDE_DIRS} ${ASIO_INCLUDE_DIRS}
    ${MSGPACK_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR} ${WEBSOCKETPP_INCLUDE_DIRS} ${ASIO_INCLUDE_DIRS}
    ${MSGPACK_INCLUDE_DIRS})


