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

# 공유 라이브러리 빌드
add_library(Objects SHARED 
    src/RawChannel.cc 
    src/RawData.cc
    src/FadcBD.cc 
    src/RunInfo.cc
    src/Pmt.cc
    src/Run.cc
    ObjectsDict.cxx
)

# [핵심 수정] 딕셔너리 컴파일 시 "include/..." 경로를 찾을 수 있도록 현재 소스 폴더 경로 추가
target_include_directories(Objects PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Objects PUBLIC ${ROOT_LIBRARIES})