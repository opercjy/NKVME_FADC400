cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(N400FADC_VME CXX)

# 실행 파일들을 최상위 bin 폴더로 자동 출력
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# C++17 표준 강제
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 컴파일 경고 활성화
add_compile_options(-Wall -Wextra -Wno-unused-parameter)

# ROOT 6 프레임워크 탐색
find_package(ROOT REQUIRED COMPONENTS Core RIO Tree Net Thread Gui MathCore Hist)
include(${ROOT_USE_FILE})

# NKHOME 환경 변수 확인
if(NOT DEFINED ENV{NKHOME})
    message(FATAL_ERROR "NKHOME environment variable is not set. Please run 'source setup.sh'")
endif()
set(NKHOME $ENV{NKHOME})

# 전역 Include & Link 경로 설정
include_directories(${ROOT_INCLUDE_DIRS} ${NKHOME}/include)
link_directories(${NKHOME}/lib)

# 하위 모듈 빌드 트리 추가
add_subdirectory(objects)
add_subdirectory(frontend)
add_subdirectory(display)

# 설정 파일 및 시스템 룰 폴더를 빌드 디렉터리로 자동 복사
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/rules DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

message(STATUS "Config and Rules directories copied to build folder.")
