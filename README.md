
# 🚀 NKVME_FADC400 : NoticeDAQ Central Control (Ultimate Edition)

**NKVME_FADC400**은 VME 기반 Notice FADC400 (400MHz, 2.5ns/sample) 보드를 제어하고 초고속으로 데이터를 수집, 모니터링, 오프라인 분석까지 수행할 수 있는 **통합 데이터 획득(DAQ) 및 분석 프레임워크**입니다. 

C++ 기반의 고성능 멀티스레드 코어와 PyQt5 기반의 모던 대시보드 GUI가 결합되어, 실제 가속기 및 고에너지 물리 실험실 통제실(Control Room) 수준의 안정성과 편의성을 제공합니다.

---

## ✨ 핵심 기능 및 최신 업데이트 (Ultimate Edition)

### 1. 🖥️ Python PyQt5 모던 대시보드 (GUI)
* **2계층(Nested) 탭 아키텍처:** 좁은 모니터 환경에서도 모든 기능(DAQ, HV, TLU, Config, Production)을 한눈에 제어할 수 있는 타이트한 레이아웃 적용.
* **실시간 대시보드:** 수집된 이벤트 수, 진행 시간(Elapsed Time), **초당 트리거 처리율(Trigger Rate, Hz)**, 버퍼 큐(DataQ/Pool) 상태, 잔여 디스크 용량을 실시간으로 시각화.
* **ANSI 컬러 콘솔 파싱:** C++ 코어에서 발생하는 시스템 로그, 경고, 에러(FATAL)의 이스케이프 컬러 코드를 HTML로 파싱하여 가독성 높은 시스템 콘솔 제공.

### 2. ⚙️ C++ 고성능 DAQ 백엔드 (`frontend_nfadc400`)
* **멀티스레딩 & 객체 풀링(Object Pool):** 생산자(하드웨어 읽기)와 소비자(디스크 쓰기/네트워크 전송) 스레드를 완벽히 분리하고, 메모리 재할당(new/delete) 없이 빈 객체를 재활용하여 램 파편화 및 병목 현상(OOM) 방지.
* **하드웨어 방어 로직:** 보드 전원 꺼짐, 케이블 분리, 물리적 스위치(Module ID)와 소프트웨어 설정 불일치 시 즉각적인 FATAL 에러를 발생시켜 잘못된 메모리 접근 및 시스템 다운(Segmentation Fault) 원천 차단.
* **독립 네트워크 타이머:** `<chrono>`를 이용한 독립 타이머로 50ms마다 온라인 모니터로 데이터를 브로드캐스팅하여 TStopwatch 충돌 및 TCP 버퍼 폭발 방지.

### 3. 📦 오프라인 분석기 (`production_nfadc400`)
* **물리적 데이터 환산 로직:** 페데스탈(Baseline) 자동 계산, 전압 강하(Voltage Drop) 펄스 반전, 적분 전하량(Charge) 및 400MHz 샘플링 속도에 맞춘 물리적 시간(2.5ns/sample) 매핑 적용.
* **배치(Batch) 모드:** 원본 `.root` 파일을 읽어 전하합 히스토그램과 분석된 데이터를 포함한 `_prod.root` 파일을 고속으로 생성. (`-w` 옵션 시 물리 단위로 환산된 파형 시계열 전체를 `std::vector<double>`로 Tree에 추가 저장)
* **인터랙티브 뷰어(-d 모드):** GUI 패널의 [Prev], [Next], [Jump] 버튼과 연동되어 개별 이벤트 파형과 분석 결과를 육안으로 검증 가능.

### 4. 🗄️ E-Logbook 데이터베이스 (`elog_manager.py`)
* SQLite3 기반의 전자 로그북(E-Logbook) 모듈 완벽 분리.
* DAQ Run이 종료되는 즉시 C++ 코어의 서머리 데이터(총 이벤트, 소요 시간, 평균 처리율)를 파싱하여 데이터베이스에 영구 기록.

---

## 📂 디렉토리 구조 (Directory Structure)

```text
NKVME_FADC400/
├── bin/                       # 빌드된 실행 파일 (.exe) 및 GUI 스크립트 복사본
├── build/                     # CMake 빌드 디렉토리
├── config/                    # DAQ 하드웨어 설정 파일 (*.config) 모음
├── display/                   # 온라인 모니터링 (ROOT GUI) 소스 코드
├── frontend/                  # C++ DAQ 백엔드 및 오프라인 분석(Production) 소스 코드
├── gui/                       # PyQt5 기반 Python 대시보드 및 모듈
│   ├── daq_gui.py             # 메인 대시보드 실행 파일
│   ├── config_manager.py      # 설정 파일 관리 모듈
│   ├── elog_manager.py        # SQLite3 E-Logbook DB 관리 모듈
│   ├── prod_manager.py        # 오프라인 분석(Production) GUI 모듈
│   ├── hv_control.py          # High Voltage 제어 모듈
│   └── tlu_simulator.py       # Trigger Logic Unit 시뮬레이터 모듈
├── nfadc400/                  # Notice FADC400 및 VME 통신 C/C++ 라이브러리 소스
├── objects/                   # ROOT Dictionary 파싱을 위한 공통 데이터 클래스 (RawData, RunInfo 등)
└── rules/                     # Linux udev 권한 설정 스크립트 (setup_usb.sh)
```

---

## 🛠️ 설치 및 빌드 방법 (Build & Installation)

### 1. 필수 의존성 (Prerequisites)
* **CERN ROOT 6** (C++11 이상 지원)
* **Python 3.x** 및 **PyQt5** (`pip install PyQt5`)
* **Notice FADC400 USB/VME 드라이버** 및 Linux 권한 (`libusb-1.0-0-dev`)

### 2. 하드웨어 권한 설정 (최초 1회)
USB-VME 컨트롤러에 일반 유저 권한으로 접근하기 위해 udev 룰을 적용합니다.
```bash
cd rules/
sudo ./setup_usb.sh
```

### 3. 컴파일 및 빌드 (CMake)
```bash
mkdir build && cd build
cmake ..
make -j4
```
> 빌드가 완료되면 `bin/` 폴더에 `frontend_nfadc400`, `OnlineMonitor_nfadc400`, `production_nfadc400` 및 Python GUI 파일들이 생성됩니다.

---

## 🚀 실행 방법 (Usage)

### 1. 통합 대시보드 실행 (GUI 권장)
모든 DAQ 수집, 모니터링, 분석 작업은 중앙 통제 대시보드에서 마우스 클릭만으로 수행할 수 있습니다.
```bash
cd bin/
./daq_gui.py
```
1. 탭에서 `[📊 Start Monitor]`를 클릭하여 온라인 뷰어를 먼저 엽니다.
2. `[▶ Start Manual DAQ]`를 클릭하여 데이터 수집을 시작합니다.
3. 수집이 끝나면 `[📦 Offline Production]` 탭으로 이동하여 분석을 수행합니다.

### 2. 수동 CLI 실행 모드
GUI를 사용하지 않고 스크립트나 터미널에서 백엔드만 개별 실행할 수도 있습니다.

**[데이터 수집 (Frontend)]**
```bash
./bin/frontend_nfadc400 -f config/ch1.config -o data.root -n 5000
```
**[온라인 모니터 (Display)]**
```bash
./bin/OnlineMonitor_nfadc400
```
**[오프라인 분석 (Production)]**
```bash
# 배치 모드 (히스토그램 및 파형 전체(-w) 저장)
./bin/production_nfadc400 -i data.root -w

# 인터랙티브 뷰어 모드 (-d)
./bin/production_nfadc400 -i data.root -d
```