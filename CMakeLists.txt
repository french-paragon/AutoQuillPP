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

set(AUTOQUILL_SDK_NAME "AutoQuillPP")
set(AUTOQUILL_SDK_EXPORTNAME "AutoQuillPPTargets")

set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}:$ORIGIN/../../${CMAKE_INSTALL_LIBDIR}")

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

#installing

install(EXPORT ${AUTOQUILL_SDK_EXPORTNAME}
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/cmake/${AUTOQUILL_SDK_NAME}
  NAMESPACE ${AUTOQUILL_SDK_NAME}::
  FILE AutoQuillSDKTargets.cmake # ensure the name is different than the AutoQuillSDKConfig.cmake file
  )

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "cmake/Config.cmake.in"
  "${AUTOQUILL_SDK_NAME}Config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/cmake/${AUTOQUILL_SDK_NAME}
  PATH_VARS
    CMAKE_INSTALL_LIBDIR
  )

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${AUTOQUILL_SDK_NAME}ConfigVersion.cmake"
  VERSION "0.0"
  COMPATIBILITY AnyNewerVersion
)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/${AUTOQUILL_SDK_NAME}Config.cmake
	${CMAKE_CURRENT_BINARY_DIR}/${AUTOQUILL_SDK_NAME}ConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/cmake/${AUTOQUILL_SDK_NAME} )
