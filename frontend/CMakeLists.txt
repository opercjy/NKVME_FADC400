# 스레드 패키지 로드
find_package(Threads REQUIRED)

add_executable(frontend 
    src/frontend.cc 
    src/ELog.cc
    src/ConfigParser.cc
)

target_include_directories(frontend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Objects 라이브러리와 Notice 하드웨어 래퍼 라이브러리 링킹
target_link_libraries(frontend PRIVATE 
    Objects 
    NoticeNFADC400ROOT 
    Notice6UVMEROOT
    Threads::Threads
    ${ROOT_LIBRARIES}
)
