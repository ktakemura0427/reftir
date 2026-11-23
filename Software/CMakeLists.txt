cmake_minimum_required(VERSION 3.16)

project(RFTIR_Singledevice_Postprocess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# If OpenCV is not found, pass -DOpenCV_DIR=<folder containing OpenCVConfig.cmake>.
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)

function(rftir_set_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /utf-8)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra)
  endif()
endfunction()

add_library(rftir_postprocess STATIC
  src/dbscan.cpp
  src/io.cpp
  src/lut_reconstructor.cpp
  src/options.cpp
  src/visualization.cpp)
target_include_directories(rftir_postprocess PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${OpenCV_INCLUDE_DIRS})
target_link_libraries(rftir_postprocess PUBLIC ${OpenCV_LIBS})
# Lets the program find the default LUT (LUT/ in this folder) when --lut is omitted.
target_compile_definitions(rftir_postprocess PRIVATE RFTIR_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
rftir_set_warnings(rftir_postprocess)

add_executable(Singledevice_postprocess main.cpp)
target_link_libraries(Singledevice_postprocess PRIVATE rftir_postprocess)
rftir_set_warnings(Singledevice_postprocess)
