# ==============================================================================
# NKVME_FADC400 Objects / Dictionary CMake (Modern ROOT 6 Syntax)
# ==============================================================================

file(GLOB SOURCES "src/*.cc")
file(GLOB HEADERS "include/*.hh")

# 1. [최신 문법] ROOT 딕셔너리 생성
root_generate_dictionary(ObjectsDict 
    ${HEADERS} 
    LINKDEF include/LinkDef.h
)

# 2. [오프라인 분석용] 공유 라이브러리 (.so) 생성 - 이름 변경!
add_library(NKVME_FADC400 SHARED ${SOURCES} ObjectsDict.cxx)
target_include_directories(NKVME_FADC400 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(NKVME_FADC400 PUBLIC ROOT::Core ROOT::RIO ROOT::Hist ROOT::Tree ROOT::MathCore)


# 3. [DAQ 코어용] 정적 라이브러리 (.a) 생성
add_library(objects STATIC ${SOURCES} ObjectsDict.cxx)
target_include_directories(objects PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(objects PUBLIC ROOT::Core ROOT::RIO ROOT::Hist ROOT::Tree ROOT::MathCore)


# 4. 빌드 완료 후 lib/ 폴더로 자동 배포
add_custom_command(TARGET NKVME_FADC400 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:NKVME_FADC400> ${CMAKE_SOURCE_DIR}/lib/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/ObjectsDict_rdict.pcm ${CMAKE_SOURCE_DIR}/lib/
    COMMENT "✅ Deploying libNKVME_FADC400.so and ObjectsDict_rdict.pcm to lib/ directory."
)
