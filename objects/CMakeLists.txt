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

# 하위 모듈이 이 라이브러리를 링크할 때 include 폴더를 자동으로 찾게 함
target_include_directories(Objects PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(Objects PUBLIC ${ROOT_LIBRARIES})
