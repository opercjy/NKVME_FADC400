cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(NoticeDAQ CXX)

# 실행 파일들을 최상위 bin 폴더로 자동 출력
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# C++17 표준 강제
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wno-unused-parameter)

find_package(ROOT REQUIRED COMPONENTS Core RIO Tree Net Thread Gui MathCore Hist)
include(${ROOT_USE_FILE})

if(NOT DEFINED ENV{NKHOME})
    message(FATAL_ERROR "NKHOME environment variable is not set. Please run 'source setup.sh'")
endif()
set(NKHOME $ENV{NKHOME})

include_directories(${ROOT_INCLUDE_DIRS} ${NKHOME}/include)
link_directories(${NKHOME}/lib)

add_subdirectory(objects)
add_subdirectory(frontend)
add_subdirectory(display)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/rules DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

message(STATUS "Config and Rules directories copied to build folder.")
