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

# 하위 C++ 모듈 빌드
add_subdirectory(objects)
add_subdirectory(frontend)
add_subdirectory(display)

# 설정 파일 및 시스템 룰 폴더 복사
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/rules DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# [핵심] GUI 파이썬 스크립트들을 모두 bin 폴더로 복사하고 실행 권한(+x) 자동 부여
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/gui/daq_gui.py 
          ${CMAKE_CURRENT_SOURCE_DIR}/gui/hv_control.py 
          ${CMAKE_CURRENT_SOURCE_DIR}/gui/tlu_simulator.py
          ${CMAKE_CURRENT_SOURCE_DIR}/gui/config_manager.py
          ${CMAKE_CURRENT_SOURCE_DIR}/gui/prod_manager.py
          ${CMAKE_CURRENT_SOURCE_DIR}/gui/elog_manager.py
          ${CMAKE_CURRENT_SOURCE_DIR}/gui/path_controller.py
          ${CMAKE_CURRENT_SOURCE_DIR}/gui/daq_widgets.py  
     DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

# 복사된 daq_gui.py의 이름을 fadc400_gui 로 깔끔하게 변경!
file(RENAME ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/daq_gui.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fadc400_gui)

message(STATUS "Config, Rules, and GUI scripts successfully copied to build folder.")
message(STATUS "🚀 GUI Executable created: bin/fadc400_gui")