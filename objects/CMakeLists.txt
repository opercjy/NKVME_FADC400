# ROOT 딕셔너리 소스 자동 생성
ROOT_GENERATE_DICTIONARY(ObjectsDict 
    include/RawChannel.hh 
    include/RawData.hh
    include/FadcBD.hh 
    include/RunInfo.hh 
    include/Pmt.hh
    include/Run.hh
    LINKDEF include/LinkDef.h
)

# 공유 라이브러리(대문자 Objects) 빌드
add_library(Objects SHARED 
    src/RawChannel.cc 
    src/RawData.cc
    src/FadcBD.cc 
    src/RunInfo.cc
    src/Pmt.cc
    src/Run.cc
    ObjectsDict.cxx
)

# 딕셔너리 컴파일 시 "include/..." 경로를 찾을 수 있도록 현재 소스 폴더 경로 추가
target_include_directories(Objects PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Objects PUBLIC ${ROOT_LIBRARIES})

# =======================================================================
# [자동화] 빌드 완료 직후(POST_BUILD) 생성된 .pcm 파일을 bin/ 으로 자동 복사
# =======================================================================
add_custom_command(TARGET Objects POST_BUILD
    COMMAND cp -u ${CMAKE_CURRENT_BINARY_DIR}/*.pcm ${CMAKE_SOURCE_DIR}/bin/
    COMMENT "Deploying Objects ROOT PCM dictionaries to bin/ directory..."
)