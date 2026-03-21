# 스레드 패키지 로드
find_package(Threads REQUIRED)

# 1. Frontend 메인 수집 엔진 빌드
add_executable(frontend_nfadc400 
    src/frontend.cc 
    src/ELog.cc
    src/ConfigParser.cc
)
target_include_directories(frontend_nfadc400 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(frontend_nfadc400 PRIVATE 
    Objects 
    NoticeNFADC400ROOT 
    Notice6UVMEROOT
    Threads::Threads
    ${ROOT_LIBRARIES}
)

# 2. Production 오프라인 분석기 빌드
add_executable(production_nfadc400 
    src/production.cc 
    src/ELog.cc
)
target_include_directories(production_nfadc400 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(production_nfadc400 PRIVATE 
    Objects 
    ${ROOT_LIBRARIES}
)
