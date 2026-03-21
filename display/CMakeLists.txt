# ROOT 딕셔너리 소스 자동 생성
ROOT_GENERATE_DICTIONARY(MonitorDict 
    include/OnlineMonitor.hh 
    LINKDEF include/MonitorLinkDef.h
)

# 실행 파일 생성 (단 한 번만 선언해야 합니다!)
add_executable(OnlineMonitor_nfadc400 
    src/OnlineMonitor.cc 
    MonitorDict.cxx
)

# 딕셔너리 컴파일 시 "include/..." 경로를 찾을 수 있도록 현재 폴더 경로 추가
target_include_directories(OnlineMonitor_nfadc400 PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 라이브러리 링킹 (대문자 Objects 로 통일)
target_link_libraries(OnlineMonitor_nfadc400 PRIVATE Objects ${ROOT_LIBRARIES})

# =======================================================================
# [자동화] 빌드 완료 직후(POST_BUILD) 생성된 .pcm 파일을 bin/ 으로 자동 복사
# =======================================================================
add_custom_command(TARGET OnlineMonitor_nfadc400 POST_BUILD
    COMMAND cp -u ${CMAKE_CURRENT_BINARY_DIR}/*.pcm ${CMAKE_SOURCE_DIR}/bin/
    COMMENT "Deploying OnlineMonitor ROOT PCM dictionaries to bin/ directory..."
)