#ifdef __CINT__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

// STL std::vector 직렬화 필수 (RawChannel 내부에서 사용)
#pragma link C++ class std::vector<unsigned short>+;

// 커스텀 데이터 객체들
#pragma link C++ class RawChannel+;
#pragma link C++ class RawData+;
#pragma link C++ class FadcBD+;
#pragma link C++ class RunInfo+;
#pragma link C++ class Pmt+;
#pragma link C++ class Run+;
#endif
