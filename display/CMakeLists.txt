# GUI 객체용 딕셔너리 생성
ROOT_GENERATE_DICTIONARY(MonitorDict 
    include/OnlineMonitor.hh 
    LINKDEF include/MonitorLinkDef.h
)

add_executable(OnlineMonitor 
    src/OnlineMonitor.cc 
    MonitorDict.cxx
)

target_include_directories(OnlineMonitor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 상위 폴더의 Objects 라이브러리와 ROOT 기본 라이브러리 링크
target_link_libraries(OnlineMonitor PRIVATE Objects ${ROOT_LIBRARIES})
