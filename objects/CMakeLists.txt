# ==============================================================================
# NKVME_FADC400 Objects / Dictionary CMake (Modern ROOT 6 Syntax)
# ==============================================================================

file(GLOB SOURCES "src/*.cc")
file(GLOB HEADERS "include/*.hh")

# 1. ROOT 딕셔너리 생성 (이름을 통합 라이브러리와 맞춤)
root_generate_dictionary(NKVME_FADC400_Dict 
    ${HEADERS} 
    LINKDEF include/LinkDef.h
)

# 2. 통합 공유 라이브러리(.so) 하나만 생성 (Static 라이브러리 삭제)
add_library(NKVME_FADC400 SHARED ${SOURCES} NKVME_FADC400_Dict.cxx)
target_include_directories(NKVME_FADC400 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(NKVME_FADC400 PUBLIC ROOT::Core ROOT::RIO ROOT::Hist ROOT::Tree ROOT::MathCore)

# 3. 빌드 완료 후 lib/ 폴더로 자동 배포 (.so, .pcm, .rootmap)
add_custom_command(TARGET NKVME_FADC400 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:NKVME_FADC400> ${CMAKE_SOURCE_DIR}/lib/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libNKVME_FADC400_Dict_rdict.pcm ${CMAKE_SOURCE_DIR}/lib/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libNKVME_FADC400_Dict.rootmap ${CMAKE_SOURCE_DIR}/lib/
    COMMENT "Deploying libNKVME_FADC400.so and Dictionary files to lib/ directory."
)