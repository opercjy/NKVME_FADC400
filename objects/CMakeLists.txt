# ==============================================================================
# NKVME_FADC400 Objects / Dictionary CMake (Modern ROOT 6 Syntax)
# ==============================================================================

file(GLOB SOURCES "src/*.cc")
file(GLOB HEADERS "include/*.hh")

# 1. ROOT 딕셔너리 생성
# 💡 [핵심 버그 픽스] 타겟명에 'G__' 를 붙여주면, ROOT가 알아서 MODULE 이름에 맞춰 
# libNKVME_FADC400_rdict.pcm 과 libNKVME_FADC400.rootmap 으로 예쁘게 이름을 통일해 줍니다!
root_generate_dictionary(G__NKVME_FADC400 
    ${HEADERS} 
    LINKDEF include/LinkDef.h
    MODULE NKVME_FADC400
)

# 2. 통합 공유 라이브러리(.so) 하나만 생성
add_library(NKVME_FADC400 SHARED ${SOURCES} G__NKVME_FADC400.cxx)
target_include_directories(NKVME_FADC400 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(NKVME_FADC400 PUBLIC ROOT::Core ROOT::RIO ROOT::Hist ROOT::Tree ROOT::MathCore)

# 3. 빌드 완료 후 lib/ 폴더로 자동 배포
add_custom_command(TARGET NKVME_FADC400 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:NKVME_FADC400> ${CMAKE_SOURCE_DIR}/lib/
    # 💡 [핵심 버그 픽스] 복사하는 타겟 파일 이름에서도 '_Dict' 꼬리표를 떼어냅니다.
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libNKVME_FADC400_rdict.pcm ${CMAKE_SOURCE_DIR}/lib/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libNKVME_FADC400.rootmap ${CMAKE_SOURCE_DIR}/lib/
    COMMENT "Deploying libNKVME_FADC400.so and Dictionary files to lib/ directory."
)