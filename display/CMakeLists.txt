# ROOT 딕셔너리 소스 자동 생성
ROOT_GENERATE_DICTIONARY(MonitorDict 
    include/OnlineMonitor.hh 
    LINKDEF include/MonitorLinkDef.h
)

add_executable(OnlineMonitor_nfadc400 
    src/OnlineMonitor.cc 
    MonitorDict.cxx
)

target_include_directories(OnlineMonitor_nfadc400 PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/objects/include
)

# 통합 공유 라이브러리(NKVME_FADC400) 링크
target_link_libraries(OnlineMonitor_nfadc400 PRIVATE 
    NKVME_FADC400 
    ${ROOT_LIBRARIES}
)

# [자동화] 생성된 MonitorDict의 .pcm 파일을 bin/ 으로 복사
add_custom_command(TARGET OnlineMonitor_nfadc400 POST_BUILD
    COMMAND cp -u ${CMAKE_CURRENT_BINARY_DIR}/libMonitorDict_rdict.pcm ${CMAKE_SOURCE_DIR}/bin/
    COMMENT "Deploying OnlineMonitor ROOT PCM dictionary to bin/ directory..."
)