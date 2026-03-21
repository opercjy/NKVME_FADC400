# GUI 객체용 딕셔너리 생성
ROOT_GENERATE_DICTIONARY(MonitorDict 
    include/OnlineMonitor.hh 
    LINKDEF include/MonitorLinkDef.h
)

add_executable(OnlineMonitor_nfadc400 
    src/OnlineMonitor.cc 
    MonitorDict.cxx
)

target_include_directories(OnlineMonitor_nfadc400 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(OnlineMonitor_nfadc400 PRIVATE Objects ${ROOT_LIBRARIES})
