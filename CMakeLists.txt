cmake_minimum_required(VERSION 3.10)
project(AutoQuillPP LANGUAGES CXX)

cmake_policy(SET CMP0023 NEW)

set (PROJ_ORG_NAME "Paragon")
set (PROJ_ORG_DOMAIN "famillejospin.ch")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Some stuff for installation
include(GNUInstallDirs)

set(CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR})
set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
set(CMAKE_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

set(AUTOQUILL_SDK_NAME "AutoQuillPPSdk")
set(AUTOQUILL_SDK_EXPORTNAME "AutoQuillPPSdkTargets")

#find usefull libraries
find_package(Qt5 COMPONENTS Widgets Core Test PrintSupport Svg REQUIRED)

#configure some variables
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

set(LIB_NAME "lib${PROJECT_NAME}")
set(PROG_NAME "${PROJECT_NAME}")

#configure library
add_subdirectory(lib)

#configure app
add_subdirectory(app)
