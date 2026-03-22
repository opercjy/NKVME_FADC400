-----

# NKVME\_FADC400 : NoticeDAQ Central Control

Notice & License
본 레포지토리는 하드웨어 제조사인 Notice Korea의 공식적인 동의 및 허가 하에 로우레벨 통신 라이브러리(nfadc400, 6uvme)의 소스 코드를 공유하며, 연구 목적의 프레임워크 통합 및 지속적인 유지보수를 수행하고 있습니다.

**NKVME\_FADC400**은 VME 기반 Notice FADC400 (400MHz, 2.5ns/sample) 보드를 제어하고 데이터를 수집, 모니터링, 오프라인 분석을 수행하는 통합 데이터 획득(DAQ) 프레임워크입니다.

본 시스템은 데이터 처리는 C++ ROOT 백엔드에서, 사용자 인터페이스와 DB 제어는 Python PyQt5 프론트엔드에서 수행하는 하이브리드 아키텍처로 설계되어 안정성과 유지보수 편의성을 제공합니다.

-----

## 1\. 시스템 아키텍처 (System Architecture)

본 프레임워크는 4개의 논리적 계층으로 구성되어 있습니다.

1.  **Hardware / Vendor Layer (`nfadc400`):** Notice Korea에서 제공한 USB-VME 브릿지, FADC400 통신, VME 상태창 관련 C/C++ 로우레벨 라이브러리.
2.  **C++ DAQ Core Layer (`frontend` & `objects`):** 하드웨어를 제어하고, 병렬 스레드(Producer-Consumer)로 디스크(ROOT 파일) 기록 및 TCP 소켓으로 브로드캐스팅하는 엔진.
3.  **C++ DQM Layer (`display`):** 프론트엔드 데이터를 수신하여 실시간으로 물리량(Baseline, Charge)을 환산하고 캔버스에 렌더링하는 실시간 모니터링 모듈.
4.  **Python Master UI Layer (`gui`):** C++ 백엔드 프로세스의 라이프사이클을 통제하고, SQLite DB, HV, TLU 제어 등을 관리하는 중앙 대시보드.

-----

## 2\. 핵심 기능 및 상세 스펙 (Core Features)

### 2.1 실시간 DQM (Data Quality Monitor) 및 동적 렌더링

  * **실시간 물리량 환산:** 파형(Waveform) 데이터를 백그라운드에서 실시간 분석하여 Baseline(첫 20샘플 평균)을 계산하고 적분 전하량(Charge)을 산출합니다.
  * **에너지 스펙트럼:** 상단 패드에는 실시간 파형을, 하단 패드에는 전하량 누적 스펙트럼 히스토그램을 표출합니다. (Log-Y 스케일 자동 적용).
  * **동적 캔버스 분할:** 수신된 패킷에서 활성화된(Nch) 채널 수를 분석하여 1x2, 2x2, 4x2 등 최적의 레이아웃으로 캔버스를 동적 재조립합니다.
  * **Auto-Reconnect:** 데이터 획득 중 모니터 프로세스가 재시작되어도, C++ 코어가 이를 감지하여 2초 주기로 자동 재연결을 수행합니다.

### 2.2 Python PyQt5 대시보드 (GUI)

  * **모듈화 탭 아키텍처:** 모든 기능(DAQ, HV, TLU, Config, Production, E-Log)을 탭 레이아웃으로 구성하여 접근성을 높였습니다.
  * **실시간 대시보드:** 수집된 이벤트 수, 진행 시간(Elapsed Time), 초당 트리거 처리율(Trigger Rate, Hz), 버퍼 큐(DataQ/Pool) 상태를 시각화합니다.
  * **동적 창 제어:** 플랫(Flat) 디자인을 적용하였으며, `[▶ Hide Dashboard]` 버튼 클릭 시 메인 제어 창의 가로 크기를 축소하여 모니터 공간 활용도를 높일 수 있습니다. (QSplitter 적용)
  * **ANSI 컬러 콘솔 파싱:** C++ 코어에서 발생하는 시스템 로그 및 에러의 이스케이프 컬러 코드를 파싱하여 시스템 콘솔에 표출합니다.

### 2.3 C++ DAQ 백엔드 (`frontend_nfadc400`) & 빌드 자동화

  * **객체 풀링(Object Pool):** 생산자(하드웨어 읽기)와 소비자(디스크 쓰기/네트워크 전송) 스레드를 분리하고, 메모리 재할당 없이 객체를 재활용하여 램 파편화 및 병목 현상을 방지합니다.
  * **하드웨어 예외 처리:** 보드 전원, 케이블 분리, 스위치(Module ID) 불일치 시 에러를 발생시켜 비정상적인 메모리 접근을 차단합니다.
  * **동기화 타이머 (`SafeTimer`):** ROOT JIT(Cling) 환경에서 발생하는 동적 바인딩 에러 및 GUI 리사이징 데드락을 방지하기 위해 컴파일 타임 바인딩 타이머를 적용했습니다.
  * **CMake 빌드 자동화:** 빌드 시 생성되는 `.pcm` (ROOT Dictionary) 파일들을 `bin/` 디렉토리로 자동 복사하도록 구성하여 런타임 링킹 에러를 방지했습니다.

### 2.4 오프라인 분석기 (`production_nfadc400`)

  * **물리 데이터 환산:** 페데스탈 차감 및 400MHz 샘플링 속도에 맞춘 시간(2.5ns/sample) 매핑을 적용합니다.
  * **배치(Batch) 모드:** 원본 `.root` 파일을 읽어 분석된 데이터와 히스토그램을 포함한 `_prod.root` 파일을 일괄 생성합니다.
  * **인터랙티브 뷰어 (-d 모드):** GUI 패널을 통해 개별 이벤트 파형과 분석 결과를 순차적으로 검증할 수 있습니다.

### 2.5 E-Logbook 데이터베이스 (`elog_manager.py`)

  * SQLite3 기반의 전자 로그북(E-Logbook) 모듈을 지원합니다.
  * DAQ 종료 시 C++ 코어의 요약 데이터(총 이벤트, 소요 시간, 평균 처리율)를 파싱하여 데이터베이스에 기록합니다.
  * `PHYSICS`, `CALIBRATION` 등의 태그 및 데이터 품질(Quality)을 지정하여 이력을 관리할 수 있습니다.

-----

## 3\. 디렉토리 구조 및 소스코드 명세 (Directory Details)

```text
NKVME_FADC400/
├── bin/                       # 빌드된 C++ 실행 파일(.exe) 및 Python GUI 스크립트 모음
├── build/                     # CMake 빌드 디렉토리
├── config/                    # DAQ 하드웨어 설정 파일 (*.config) 모음
├── lib/                       # 오프라인 분석용 공유 라이브러리(.so) 및 ROOT 딕셔너리(.pcm)
│
├── gui/                       # [Python] 메인 통제 대시보드 및 서브 모듈
│   ├── daq_gui.py             # 중앙 대시보드 (메인 런처). 프로세스 제어 및 UI 담당
│   ├── elog_manager.py        # SQLite3 E-Logbook(daq_history.db) 관리 모듈
│   ├── config_manager.py      # 설정 파일 및 데이터 저장 경로 관리 모듈
│   ├── prod_manager.py        # 오프라인 분석(Production) GUI 모듈
│   ├── hv_control.py          # High Voltage 제어 모듈
│   └── tlu_simulator.py       # Trigger Logic Unit 시뮬레이터 모듈
│
├── frontend/                  # [C++] 데이터 수집 코어 모듈
│   ├── src/frontend.cc        # 메인 DAQ 백엔드. 멀티스레드, Object Pool, 하드웨어 제어
│   ├── src/production.cc      # 오프라인 데이터 환산 분석기 (_prod.root 생성)
│   ├── src/ConfigParser.cc    # 텍스트 설정 파일을 C++ 객체로 변환
│   └── src/ELog.cc            # 터미널용 로그 출력 유틸리티
│
├── display/                   # [C++] 실시간 DQM (Data Quality Monitor)
│   └── src/OnlineMonitor.cc   # TCP 데이터 수신, 파형 및 스펙트럼 동적 렌더링
│
├── objects/                   # [C++] ROOT 딕셔너리 공유 객체
│   └── src/(RawData, RunInfo, RawChannel, Pmt, Run).cc # 프로세스 간 통신용 데이터 컨테이너
│
├── nfadc400/                  # [C/C++] Notice 제조사 제공 로우레벨 라이브러리 소스
│   ├── src/nfadc400/          # FADC400 레지스터 맵 및 제어 함수
│   ├── src/6uvme/             # USB-VME 브릿지 통신 라이브러리
│   └── src/display/           # VME 상태창 (NoticeDISPLAY) 모듈
│
├── rules/                     # Linux 환경 USB 장치 권한 설정 스크립트 (setup_usb.sh)
│
├── setup.sh                   # 프로젝트 통합 환경 변수(ROOT, 라이브러리 경로 등) 셋업 스크립트
├── offline_charge.cpp         # 오프라인 ROOT 매크로: 범용 전하량 스펙트럼 시각화 (Log-Y)
└── offline_spe.cpp            # 오프라인 ROOT 매크로: PMT SPE(단일 광전자) Multi-Gaussian 교정
```

-----

## 4. 설치 및 빌드 방법 (Build & Installation)

### 4.1 제조사 로우레벨 라이브러리 컴파일 (★초보자 필독: 가장 먼저 수행!)
메인 프레임워크를 빌드하기 전에, 하드웨어 장비와 직접 통신하는 Notice 제조사의 원본 라이브러리를 **반드시 가장 먼저** 컴파일해야 합니다. 환경 변수를 셋업하고 3개의 하위 폴더(`6uvme`, `nfadc400`, `display`)에 각각 들어가서 빌드를 진행합니다.

```bash
# 1. 제조사 환경 변수 로드 (NKHOME 경로 지정)
source nfadc400/notice_env.sh

# 2. USB-VME 브릿지 통신 라이브러리 컴파일
cd nfadc400/src/6uvme
make clean; make

# 3. FADC400 제어 라이브러리 컴파일
cd ../nfadc400
make clean; make

# 4. VME 상태창(NoticeDISPLAY) 모듈 컴파일
cd ../display
make clean; make

# 프로젝트 최상위 루트 폴더로 복귀
cd ../../..
```

### 4.2 필수 패키지 설치
OS 환경에 맞게 시스템 의존성 패키지를 설치합니다.

**[Rocky Linux / AlmaLinux / CentOS (페도라 계열)]** - *연구소 권장 환경*
```bash
sudo dnf install epel-release
sudo dnf update
# C++ 빌드 환경, USB 드라이버, Python 개발 헤더 및 SQLite 등 설치
sudo dnf install libusb1-devel python3-pip python3-devel cmake gcc-c++ make sqlite
# GUI 및 HV 제어(CAEN)를 위한 Python 패키지 설치
pip3 install PyQt5 caen-libs
```

**[Ubuntu / Debian 계열]**
```bash
sudo apt-get update
# C++ 빌드 환경, USB 드라이버, Python 개발 헤더 및 SQLite 등 설치
sudo apt-get install libusb-1.0-0-dev python3-pip python3-dev cmake build-essential sqlite3
# GUI 및 HV 제어(CAEN)를 위한 Python 패키지 설치
pip3 install PyQt5 caen-libs
```
*(※ CERN ROOT 6는 시스템에 미리 컴파일 및 설치되어, `thisroot.sh`를 통해 환경 변수가 설정되어 있어야 합니다.)*

### 4.3 하드웨어 USB 권한 설정 (최초 1회)
리눅스 환경에서 일반 사용자가 USB-VME 컨트롤러에 접근할 수 있도록 udev 룰을 등록합니다.
```bash
cd rules/
sudo ./setup_usb.sh
cd ..
```

### 4.4 메인 프로젝트 통합 환경 변수 및 컴파일 (CMake)
제조사 라이브러리 세팅이 끝났다면, 메인 프로젝트의 환경 변수를 로드하고 통합 프레임워크를 빌드합니다.

```bash
# 1. 메인 프로젝트 통합 환경 변수 로드 (ROOT 및 라이브러리 경로 매핑)
source setup.sh

# 2. CMake 빌드 수행
mkdir build && cd build
cmake ..
make -j4
```
> **[💡 주의사항]**
> 터미널을 새로 열어 DAQ 작업을 시작할 때마다 반드시 프로젝트 루트 폴더에서 `source nfadc400/notice_env.sh` 와 `source setup.sh` 두 명령어를 실행하여 환경 변수를 활성화해야 정상적으로 구동됩니다.
> (빌드가 완료되면 `frontend_nfadc400`, `OnlineMonitor_nfadc400` 실행 파일과 ROOT 직렬화에 필요한 `*Dict_rdict.pcm` 파일들이 `bin/` 폴더로 자동 배포됩니다.)

-----

## 5\. 사용자 매뉴얼 (Usage)

### [GUI 모드] 통합 대시보드 실행 (권장)

GUI 대시보드를 통해 전체 시스템을 제어합니다.

```bash
cd bin/
./daq_gui.py
```

1.  좌측 패널의 **`[👁️ Show Monitor]`** 버튼을 클릭하여 실시간 모니터링 창을 엽니다.
2.  `Tag` 및 `Quality` 속성을 지정하고 \*\*`[▶ Start Manual DAQ]`\*\*를 클릭하여 데이터 획득을 시작합니다.
3.  좁은 화면 환경에서는 **`[▶ Hide Dashboard]`** 버튼을 통해 메인 윈도우 크기를 조절할 수 있습니다.
4.  수집 완료 후 **`[📦 Offline Production]`** 탭에서 데이터를 분석하거나, \*\*`[🗄️ Database & E-Log]`\*\*에서 런 이력을 관리합니다.

### [CLI 모드] 터미널 직접 실행

터미널에서 각 프로세스를 독립적으로 실행할 수 있습니다.

```bash
# 데이터 수집 (Frontend)
./bin/frontend_nfadc400 -f config/ch1.config -o data.root -n 5000

# 실시간 DQM (Online Monitor)
./bin/OnlineMonitor_nfadc400

# 오프라인 분석 (Production Batch)
./bin/production_nfadc400 -i data.root -w
```

-----

## 6\. 유지보수 가이드 (Maintenance)

시스템 구조 변경 시 다음 사항을 준수하시기 바랍니다.

### 6.1 ROOT 딕셔너리 유지보수

  * 프로세스 간 통신은 `RawData` 클래스의 직렬화를 기반으로 작동합니다.
  * `objects/include/RawData.hh` 등 공유 구조체를 수정한 경우, **반드시 `build` 디렉토리에서 `make clean` 후 재빌드**하여 딕셔너리를 갱신해야 합니다.

### 6.2 제조사 라이브러리 (`nfadc400/`) 연동

  * 펌웨어나 로우레벨 라이브러리 업데이트 시, `nfadc400/src/` 내부의 관련 C/C++ 소스를 덮어씌웁니다.
  * `frontend.cc`의 하드웨어 초기화 시퀀스는 제조사 사양을 반영한 것이므로, 임의 변경 시 타임아웃 등의 오류가 발생할 수 있습니다.

### 6.3 Python GUI 모듈 확장

  * 기능 추가 시 단일 파일(`daq_gui.py`)의 비대화를 막기 위해 기능별로 분리된 파이썬 모듈 구조를 권장합니다.
  * 신규 제어 탭은 별도의 클래스로 작성 후, `daq_gui.py` 내 `tabs.addTab()`을 통해 통합하십시오.

### 6.4 실시간 분석 알고리즘 변경

  * DQM 모니터의 Charge 적분 및 Baseline 산출 로직을 변경하려면 `display/src/OnlineMonitor.cc` 파일 내 `HandleTimer()` 함수의 데이터 처리 블록을 수정한 뒤 C++ 코드를 재컴파일해야 합니다.

## 7\. 그래픽 인터페이스 및 사용자 경험

<img width="1270" height="940" alt="image" src="https://github.com/user-attachments/assets/ced23bd3-06b2-4faa-bddb-018c39bbaae2" />
<img width="1270" height="940" alt="image" src="https://github.com/user-attachments/assets/d9ad8af2-77a2-4108-b7d8-da7460405d7d" />
<img width="1270" height="940" alt="image" src="https://github.com/user-attachments/assets/eb295e20-05c2-4e68-9caa-86f192c39625" />
<img width="1270" height="940" alt="image" src="https://github.com/user-attachments/assets/2a0c8cf5-a674-4a17-a82e-a4dad607fb50" />
<img width="1270" height="940" alt="image" src="https://github.com/user-attachments/assets/5744742d-92b1-4c99-93bf-1c13a7b1fb0e" />
<img width="1270" height="940" alt="image" src="https://github.com/user-attachments/assets/a4b91cb5-1f17-4446-84f8-1cb2a2ef987a" />
<img width="1270" height="940" alt="image" src="https://github.com/user-attachments/assets/58fc1b39-12d2-4f60-9ece-0ebbbdafc2df" />
<img width="1270" height="940" alt="image" src="https://github.com/user-attachments/assets/4565c7cf-29ef-4f8b-97dd-ce128b0a0e78" />
<img width="1270" height="940" alt="image" src="https://github.com/user-attachments/assets/4d15261c-3e59-4088-a6f9-bf66f915771f" />
<img width="1270" height="940" alt="image" src="https://github.com/user-attachments/assets/41a9c4f4-8ad6-4b8b-a15b-19cfd08ad231" />
<img width="1270" height="940" alt="image" src="https://github.com/user-attachments/assets/652bddc2-e571-4d90-8f6f-ea8ff68a622e" />
<img width="1240" height="860" alt="image" src="https://github.com/user-attachments/assets/55c9c7f6-08fa-4f04-b098-bd293ebef5b1" />
<img width="1270" height="940" alt="image" src="https://github.com/user-attachments/assets/1d313709-2eb3-426e-96ba-4dfa9459b8ca" />
<img width="1270" height="940" alt="image" src="https://github.com/user-attachments/assets/5b4f654c-dc68-4775-9a32-a2d3e674f1b2" />
<img width="1220" height="840" alt="image" src="https://github.com/user-attachments/assets/b23b6cd1-9fbb-4324-8356-56bc3e8720c9" />

## 8\. 감사의 글

*본 데이터 획득 시스템(DAQ)의 개발을 위해 하드웨어 연동 로우레벨 라이브러리(nfadc400, 6uvme)를 제공해주시고 아낌없는 기술 지원을 해주신 Notice Korea 김상열 사장님께 깊은 감사를 표합니다.

*약 15년 전남대학교에서 현재 시스템의 핵심인 데이터 직렬화 및 원천 데이터 객체(Obj) 아키텍처의 초석을 설계해 주신 기초과학연구원(IBS) 지하실험 연구단(CUP) 이재승 박사님께 각별한 감사의 말씀을 전합니다.

*데이터 분석 코어에 사용된 오픈소스 프레임워크인 CERN ROOT 개발진과, 대시보드 UI 구현에 사용된 PyQt5 커뮤니티의 기술적 기여를 인정합니다.

*무엇보다 본 프레임워크를 비롯한 기초 과학 연구의 밑바탕에는 국민 여러분께서 땀 흘려 조성해 주신 소중한 연구기금이 있습니다. 대한민국 과학 기술의 진보와 기초 학문의 발전을 위해 묵묵히, 그리고 아낌없는 지지와 성원을 보내주신 국민 여러분께 가장 깊고 진심 어린 감사의 말씀을 올립니다.





