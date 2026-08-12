cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

if(NOT ENABLE_FRONTEND_API OR NOT ENABLE_QT)
  message(FATAL_ERROR "WHIP Custom Simulcast requires the OBS Frontend API and Qt Widgets.")
endif()

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 REQUIRED COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/plugin-main.cpp
    src/plugin-config.cpp
    src/plugin-config.hpp
    src/settings-ui.cpp
    src/settings-ui.hpp
    src/simulcast-controller.cpp
    src/simulcast-controller.hpp
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

if(MSVC)
  # Avoid embedding the build machine's absolute PDB path in the distributed DLL.
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /guard:cf)
  target_link_options(${CMAKE_PROJECT_NAME} PRIVATE /guard:cf /CETCOMPAT "/PDBALTPATH:${_name}.pdb")
endif()

install(FILES LICENSE README.md README.en.md CHANGELOG.md DESTINATION "${CMAKE_PROJECT_NAME}" COMPONENT Runtime)

include(CTest)

if(BUILD_TESTING AND OS_WINDOWS)
  add_executable(plugin-config-tests tests/plugin-config-tests.cpp src/plugin-config.cpp src/plugin-config.hpp)
  target_include_directories(plugin-config-tests PRIVATE src)
  target_link_libraries(plugin-config-tests PRIVATE OBS::libobs)
  add_test(
    NAME plugin-config-tests
    COMMAND
      "${CMAKE_COMMAND}" -E env "PATH=${CMAKE_CURRENT_SOURCE_DIR}/.deps/bin/64bit;$ENV{PATH}"
      "$<TARGET_FILE:plugin-config-tests>"
  )
endif()
