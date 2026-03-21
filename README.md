# 🚀 NKVME_FADC400 : NoticeDAQ Central Control (Ultimate DQM Edition)

**NKVME_FADC400**은 VME 기반 Notice FADC400 (400MHz, 2.5ns/sample) 보드를 제어하고 초고속으로 데이터를 수집, 모니터링, 오프라인 분석까지 수행할 수 있는 **통합 데이터 획득(DAQ) 및 분석 프레임워크**입니다. 

C++ 기반의 고성능 멀티스레드 코어와 PyQt5 기반의 모던 대시보드 GUI가 결합되어 있으며, 특히 이번 **Ultimate DQM Edition**에서는 물리학자를 위한 실시간 스펙트럼 분석 및 동적 UI/UX 제어 기술이 대거 탑재되었습니다.

---

## ✨ 핵심 기능 및 최신 업데이트 (Ultimate DQM Edition)

### 1. 📈 실시간 DQM (Data Quality Monitor) 및 동적 렌더링
* **실시간 물리량 환산 (On-the-fly Analysis):** 1차원적인 파형(Waveform) 뷰어를 넘어, 백그라운드에서 실시간으로 Baseline(첫 20샘플 평균)을 계산하고, 펄스를 반전시켜 **적분 전하량(Charge)**을 도출합니다.
* **에너지 스펙트럼 (Energy Spectrum):** 화면 상단에는 파형을, 하단에는 전하량 누적 스펙트럼 히스토그램을 분할하여 표출합니다. (광전자 피크 식별을 위한 Log-Y 스케일 자동 적용)
* **동적 캔버스 리빌드 (Dynamic Canvas):** 현재 활성화된(Nch) 채널 수를 프론트엔드 패킷에서 스스로 분석하여, 1x2, 2x2, 4x2 등 **빈 캔버스 낭비 없이 최적의 레이아웃으로 스스로 캔버스를 재조립**합니다.
* **Auto-Reconnect:** 데이터 획득 중 모니터를 끄거나 다시 켜도, C++ 코어가 이를 감지하여 2초마다 자동 재연결을 수행합니다.

### 2. 🗄️ E-Logbook 데이터베이스 메인 탭 격상 및 확장
* **전체 화면 독립 패널:** 기존 대시보드 하단에 있던 E-Logbook을 널찍한 메인 탭으로 분리 독립시켜 수백 개의 런(Run) 이력을 쾌적하게 엑셀처럼 열람할 수 있습니다.
* **태그 및 품질 관리 (Tag & Quality):** 수집 시 `PHYSICS`, `CALIBRATION`, `COSMIC` 등의 물리 태그와 `GOOD`, `BAD` 등의 데이터 품질(Data Quality)을 지정하여 SQLite DB에 영구 기록하는 기능이 추가되었습니다.

### 3. 🖥️ 눈이 편안한 모던 UI/UX & 동적 창 제어
* **플랫(Flat) 디자인:** 장시간 통제실 모니터를 봐야 하는 오퍼레이터의 눈 보호를 위해, 형광색을 배제하고 차분한 파스텔 및 플랫 톤의 UI로 전면 개편했습니다.
* **대시보드 숨김 (Hide/Show Toggle):** 리눅스 윈도우 매니저의 강제 크기 제약(Minimum Size)을 우회하는 동적 리사이징을 적용하여, `[▶ Hide Dashboard]` 버튼 클릭 시 메인 창이 콤팩트하게 절반으로 접히고 펴집니다. `QSplitter`를 통한 자유로운 마우스 스와이프 조절도 지원합니다.

### 4. ⚙️ C++ 고성능 DAQ 백엔드 & 빌드 자동화
* **객체 풀링(Object Pool) & 멀티스레딩:** 생산자(하드웨어 읽기)와 소비자(디스크 쓰기/네트워크 전송) 스레드를 완벽히 분리하고 큐의 객체를 재활용하여 OOM(Out of Memory) 병목을 차단합니다.
* **ROOT 딕셔너리 에러 원천 차단 (`SafeTimer`):** ROOT JIT(Cling) 환경에서 발생하는 동적 바인딩 에러 및 리사이징 데드락(Deadlock)을 방지하기 위해 순수 C++ 컴파일 타임 바인딩 타이머를 적용했습니다.
* **CMake POST_BUILD 훅 자동화:** 빌드 완료 시 생성되는 `.pcm` (ROOT Dictionary) 파일들을 `bin/` 디렉토리로 자동 배포(Deploy)하도록 빌드 스크립트를 자동화하여 런타임 링킹 에러를 완전히 근절했습니다.

---

## 📂 디렉토리 구조 (Directory Structure)

```text
NKVME_FADC400/
├── bin/                       # 빌드된 실행 파일 (.exe), GUI 스크립트 복사본 및 .pcm 딕셔너리
├── build/                     # CMake 빌드 디렉토리
├── config/                    # DAQ 하드웨어 설정 파일 (*.config) 모음
├── display/                   # 실시간 DQM (Data Quality Monitor) C++ 소스 코드
├── frontend/                  # C++ DAQ 백엔드 및 오프라인 분석(Production) 소스 코드
├── gui/                       # PyQt5 기반 Python 대시보드 및 모듈
│   ├── daq_gui.py             # 메인 대시보드 실행 파일 (UI 제어 및 프로세스 통합)
│   ├── elog_manager.py        # SQLite3 E-Logbook DB 관리 모듈 (Tag/Quality 지원)
│   ├── config_manager.py      # 설정 파일 관리 모듈
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
> 빌드가 완료되면 `bin/` 폴더에 `frontend_nfadc400`, `OnlineMonitor_nfadc400`, `production_nfadc400` 실행 파일과 GUI 스크립트, 그리고 ROOT 연동을 위한 `.pcm` 파일들이 **자동으로 배치**됩니다.

---

## 🚀 실행 방법 (Usage)

### 1. 통합 대시보드 실행 (GUI 권장)
모든 DAQ 수집, 실시간 DQM, DB 메타데이터 기록, 오프라인 분석 작업은 중앙 통제 대시보드에서 제어됩니다.
```bash
cd bin/
./daq_gui.py
```
1. 좌측 제어부의 `[👁️ Show Monitor]`를 클릭하여 실시간 DQM 창을 엽니다.
2. `Tag` (PHYSICS, CALIBRATION 등) 및 `Quality` 옵션을 지정한 후 `[▶ Start Manual DAQ]`를 클릭하여 수집을 시작합니다.
3. 좁은 모니터 환경일 경우 `[▶ Hide Dashboard]` 버튼을 눌러 대시보드를 접고 DQM 모니터에 집중할 수 있습니다.
4. 수집이 끝난 런은 `[🗄️ Database & E-Log]` 탭에서 이력을 관리할 수 있습니다.

### 2. 수동 CLI 실행 모드
GUI 없이 스크립트나 터미널에서 백엔드를 개별 실행할 수 있습니다.

**[데이터 수집 (Frontend)]**
```bash
./bin/frontend_nfadc400 -f config/ch1.config -o data.root -n 5000
```
**[실시간 DQM (Online Monitor)]**
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
