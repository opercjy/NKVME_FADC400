# ROOT 딕셔너리 소스 자동 생성
ROOT_GENERATE_DICTIONARY(MonitorDict 
    include/OnlineMonitor.hh 
    LINKDEF include/MonitorLinkDef.h
)

add_executable(OnlineMonitor_nfadc400 
    src/OnlineMonitor.cc 
    MonitorDict.cxx
)

# [핵심 수정] 딕셔너리 컴파일 시 "include/..." 경로를 찾을 수 있도록 현재 폴더 경로 추가
target_include_directories(OnlineMonitor_nfadc400 PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(OnlineMonitor_nfadc400 PRIVATE Objects ${ROOT_LIBRARIES})