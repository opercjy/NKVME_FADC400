-----

# NKVME_FADC400 : NoticeDAQ Central Control
![C++](https://img.shields.io/badge/C++-17-blue?style=flat-square&logo=c%2B%2B)
![Python](https://img.shields.io/badge/Python-3.x-3776AB?style=flat-square&logo=python&logoColor=white)
![ROOT](https://img.shields.io/badge/Framework-CERN%20ROOT%206-005aaa?style=flat-square)
![PyQt5](https://img.shields.io/badge/GUI-PyQt5-41CD52?style=flat-square&logo=qt&logoColor=white)
![ZeroMQ](https://img.shields.io/badge/Network-ZeroMQ-DF0000?style=flat-square&logo=zeromq&logoColor=white)
![CMake](https://img.shields.io/badge/Build-CMake-064F8C?style=flat-square&logo=cmake&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Linux-FCC624?style=flat-square&logo=linux&logoColor=black)
![License](https://img.shields.io/badge/License-Notice_Authorized-red?style=flat-square)

> **Notice & License**
> 본 레포지토리는 하드웨어 제조사인 Notice Korea의 공식적인 동의 및 허가 하에 로우레벨 통신 라이브러리(nfadc400, 6uvme)의 소스 코드를 공유하며, 연구 목적의 프레임워크 통합 및 지속적인 유지보수를 수행하고 있습니다.

**NKVME\_FADC400**은 VME 기반 Notice FADC400 (400MHz, 2.5ns/sample) 보드를 제어하고 데이터를 수집, 모니터링, 오프라인 분석을 수행하는 통합 데이터 획득(DAQ) 프레임워크입니다.

본 시스템은 극도의 성능 최적화가 적용된 C++ 코어 백엔드(Zero-Deadtime & Pure Binary Dump)와 사용자 편의성 및 실시간 ZMQ 모니터링을 담당하는 Python PyQt5 프론트엔드로 구성된 완벽한 하이브리드 아키텍처로 설계되었습니다.

-----

## 1\. 시스템 아키텍처 (System Architecture)

본 프레임워크는 3개의 논리적 핵심 계층으로 구성되어 있습니다.

1.  **Hardware / Vendor Layer (`nfadc400`):** Notice Korea에서 제공한 USB-VME 브릿지, FADC400 통신 로우레벨 라이브러리 및 ROOT 연동 래퍼.
2.  **C++ DAQ Core Layer (`frontend` & `objects`):** VME 통신 병목을 부수고 하드웨어를 제어하는 초고속 엔진. 디스크에 무부하 순수 바이너리(`.dat`)를 덤프하며, 동시에 \*\*ZeroMQ(ZMQ)\*\*를 통해 파이썬 GUI로 최신 파형 패킷을 브로드캐스팅합니다.
3.  **Python Master UI & DQM Layer (`gui`):** C++ 백엔드 프로세스의 라이프사이클을 통제하고, SQLite DB 통제, HV/TLU 제어는 물론 비동기 QThread와 PyQtGraph를 이용해 CPU 점유율 없이 실시간 파형 DQM(Data Quality Monitor)을 렌더링합니다.

-----

## 2\. 핵심 기능 및 상세 스펙 (Core Features)

### 2.1 초고속 DAQ 아키텍처 및 VME 병목 타파 (High-Performance Core)

  * **Dump & Demux (Bulk-Fetch):** 과거 VME 마이크로 트랜잭션으로 인한 USB 2.0 병목을 원천 제거했습니다. 1MB 단위의 메모리 블록 전체를 단숨에 퍼온 뒤 PC의 CPU 상에서 슬라이싱(Demux)하는 아키텍처를 통해 **통신 속도를 한계치까지 극대화**했습니다.
  * **Zero-Deadtime 핑퐁 트리거:** 데이터를 읽어오기 직전(Immediately)에 반대편 하드웨어 버퍼를 선제 가동하도록 타이밍을 재배치하여, **PC가 데이터를 수집하는 동안에도 하드웨어 데드타임 0%를 달성**했습니다.
  * **Pure Binary Dump:** 온라인 수집 시 무거운 ROOT 직렬화를 배제하고, C++ 엔진이 순수 바이너리(`.dat`) 덩어리만을 디스크에 덤프하여 고속(High-rate) 이벤트 환경에서 CPU 과부하 및 데이터 유실을 완벽히 차단했습니다.

### 2.2 실시간 DQM (ZeroMQ + PyQtGraph 통합)

  * 과거의 무거운 TSocket 방식을 탈피하고 세계 최고 수준의 고속 메시지 큐인 \*\*ZeroMQ(ZMQ)\*\*를 도입했습니다. 특히 `CONFLATE` 정책을 적용하여 네트워크 백프레셔(Backpressure) 및 메모리 폭발을 완벽히 차단했습니다 (Safe-Drop).
  * 파이썬 GUI 내부의 비동기 수신 스레드와 `PyQtGraph`를 통해 DAQ 프로세스와 완벽히 분리된 초고속 라이브 파형 렌더링을 제공합니다.

### 2.3 Python PyQt5 대시보드 (GUI)

  * **모듈화 탭 아키텍처:** DAQ 통제, HV 제어, Config, 오프라인 Production, E-Log 등 모든 기능을 단일 대시보드로 통합 관리합니다.
  * **스마트 자동화 제어:** 지정된 이벤트/시간 단위로 파일을 쪼개서 수집하는 Long-Term Subrun 모드 및 자동으로 하드웨어 임계값을 조절하는 Threshold Scan 기능을 제공합니다.
  * **ANSI 컬러 콘솔 파싱:** C++ 코어에서 발생하는 시스템 로그 및 에러의 이스케이프 컬러 코드를 실시간으로 파싱하여 시스템 콘솔에 표출합니다.

### 2.4 오프라인 분석기 (`production_nfadc400`)

  * **바이너리 고속 매핑:** 수집된 가벼운 순수 바이너리(`.dat`) 파일을 읽어들여 단숨에 페데스탈 차감, 전하량(Qtot) 산출을 수행하고 무손실 ROOT 파일(`_prod.root`)로 변환합니다.
  * **인터랙티브 뷰어 (-d 모드):** 터미널 명령어를 통해 개별 이벤트 파형과 분석 베이스라인을 순차적으로 검증할 수 있는 디버깅 인터페이스를 지원합니다.

### 2.5 E-Logbook 데이터베이스 (`elog_manager.py`)

  * SQLite3 기반의 전자 로그북(E-Logbook) 모듈. DAQ 종료 시 C++ 코어의 요약 데이터(총 이벤트, 시간, 처리율)를 파싱하여 DB에 자동 기록합니다.

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
│   ├── daq_gui.py             # 중앙 대시보드 (ZMQ 기반 실시간 파형 뷰어 통합)
│   ├── elog_manager.py        # SQLite3 E-Logbook 관리 모듈
│   ├── config_manager.py      # 설정 파일 및 데이터 저장 경로 관리 모듈
│   ├── prod_manager.py        # 오프라인 분석(Production) GUI 모듈
│   ├── hv_control.py          # High Voltage 제어 모듈
│   └── tlu_simulator.py       # Trigger Logic Unit 시뮬레이터 모듈
│
├── frontend/                  # [C++] 데이터 수집 코어 모듈
│   ├── src/frontend.cc        # 메인 DAQ. VME Bulk-Fetch, Binary Dump, ZMQ 퍼블리셔
│   ├── src/production.cc      # 오프라인 분석기 (Binary -> ROOT 매핑 및 변환)
│   ├── src/ConfigParser.cc    # 텍스트 설정 파싱
│   └── src/ELog.cc            # 터미널용 로그 출력 유틸리티
│
├── display/                   # [Deprecated] 구형 TSocket 기반 ROOT 모니터 모듈 (ZMQ 도입으로 비활성화)
│
├── objects/                   # [C++] ROOT 딕셔너리 공유 객체
│   └── src/(RawData, RunInfo...).cc # 프로세스 간 통신용 데이터 컨테이너
│
├── nfadc400/                  # [C/C++] Notice 제조사 제공 로우레벨 라이브러리 소스
├── rules/                     # Linux 환경 USB 장치 권한 설정 스크립트
├── setup.sh                   # 프로젝트 통합 환경 변수 셋업 스크립트
└── offline_*.cpp              # 오프라인 ROOT 물리 분석용(SPE, Charge) 매크로 스크립트
```

-----

## 4\. 설치 및 빌드 방법 (Build & Installation)

### 4.1 제조사 로우레벨 라이브러리 컴파일 및 설치 (★초보자 필독: 가장 먼저 수행\!)

메인 프레임워크 빌드 전, 하드웨어와 직접 통신하는 **일반 코어(Core) 라이브러리**와 \*\*ROOT 연동 커널(ROOT Wrapper)\*\*을 모두 컴파일하고 시스템(`/usr/local/notice`)에 설치해야 합니다. 권한 문제 방지를 위해 반드시 `su` (또는 `sudo su`)로 최고 관리자 권한을 얻은 뒤 환경 설정을 로드하여 진행하십시오.

```bash
# 1. 관리자 권한 획득 (su 또는 sudo su)
sudo su

# 2. 제조사 환경 변수 로드 (NKHOME 경로 설정 등)
source nfadc400/notice_env.sh

# 3. USB-VME 브릿지 통신 라이브러리 설치
cd nfadc400/src/6uvme/6uvme          # 일반 C 코어
make clean; make; make install

cd ../6uvmeroot                      # ROOT 연동 커널
make clean; make; make install

# 4. FADC400 제어 라이브러리 설치
cd ../../nfadc400/nfadc400           # 일반 C 코어
make clean; make; make install

cd ../nfadc400root                   # ROOT 연동 커널
make clean; make; make install

# 5. VME 상태창 (NoticeDISPLAY) 모듈 설치
cd ../../display/display             # 일반 C 코어
make clean; make; make install

cd ../displayroot                    # ROOT 연동 커널
make clean; make; make install

# 6. 관리자 모드 종료 및 최상단 디렉토리 복귀
exit
cd ../../../..
```

### 4.2 필수 시스템 패키지 설치

ZeroMQ 라이브러리와 Python GUI 패키지를 설치합니다.

**[Rocky Linux / AlmaLinux / CentOS]** - *연구소 권장 환경*

```bash
sudo dnf install epel-release
sudo dnf update
# C++ 환경, USB, ZMQ, Python 개발 헤더 설치
sudo dnf install libusb1-devel python3-pip python3-devel cmake gcc-c++ make sqlite cppzmq-devel
# GUI 및 통신용 파이썬 패키지 설치
pip3 install pyzmq pyqtgraph PyQt5 caen-libs numpy
```

**[Ubuntu / Debian 계열]**

```bash
sudo apt-get update
# C++ 환경, USB, ZMQ, Python 개발 헤더 설치
sudo apt-get install libusb-1.0-0-dev python3-pip python3-dev cmake build-essential sqlite3 libzmq3-dev
# GUI 및 통신용 파이썬 패키지 설치
pip3 install pyzmq pyqtgraph PyQt5 caen-libs numpy
```

*(※ CERN ROOT 6는 시스템에 미리 컴파일 및 환경변수(`thisroot.sh`) 셋업이 완료되어 있어야 합니다.)*

### 4.3 하드웨어 USB 권한 설정 (최초 1회)

```bash
cd rules/
sudo ./setup_usb.sh
cd ..
```

### 4.4 메인 프로젝트 통합 빌드 (CMake)

```bash
# 통합 환경 변수 로드 (ROOT 및 라이브러리 경로 매핑)
source setup.sh

# CMake 빌드 수행
mkdir build && cd build
cmake ..
make -j4
```

> **[💡 주의사항]**
> 터미널을 새로 열 때마다 항상 프로젝트 최상단에서 `source nfadc400/notice_env.sh` 와 `source setup.sh` 를 순서대로 실행해야 정상적으로 동작합니다.

-----

## 5\. 사용자 매뉴얼 (Usage)

### [GUI 모드] 통합 대시보드 실행 (권장)

```bash
cd bin/
./daq_gui.py
```

1.  좌측 패널의 **`[👁️ Monitor ON]`** 버튼을 클릭하여 무부하 ZeroMQ 실시간 모니터링 뷰어를 개방합니다.
2.  `Run Number`, `Tag`를 지정하고 \*\*`[▶ Start Manual DAQ]`\*\*를 클릭하여 초고속 데이터 획득을 시작합니다.
3.  수집 완료 후 **`[📦 Offline Production]`** 탭에서 생성된 `.dat` 파일을 선택하고 **Run Batch**를 눌러 최종 물리 데이터(`_prod.root`)로 추출합니다.

### [CLI 모드] 터미널 직접 실행

터미널에서 각 C++ 프로세스를 독립적으로 실행할 수 있습니다.

```bash
# 데이터 수집 (고속 Binary Dump)
./bin/frontend_nfadc400 -f config/ch1.config -o data.dat -n 10000

# 오프라인 ROOT 데이터 변환
./bin/production_nfadc400 -i data.dat -o data_prod.root -w
```

-----

## 6\. 유지보수 가이드 (Maintenance)

  * **ROOT 딕셔너리 유지보수:** `RawData.hh` 등 공유 구조체를 수정한 경우, 반드시 `build` 디렉토리에서 `make clean` 후 재빌드하여 딕셔너리를 갱신해야 합니다.
  * **펌웨어/라이브러리 업데이트:** `nfadc400/src/` 내부의 원본 C 소스를 덮어씌운 후 4.1번 절차를 다시 수행하십시오.
  * **프론트엔드 최적화 코드:** `frontend.cc` 내의 `NFADC400dump_BUFFER` 루틴은 VME 통신 병목을 해소하기 위한 핵심(Bulk-Fetch) 코드이므로 임의 수정을 지양하십시오.

-----

## 7\. 그래픽 인터페이스 및 사용자 경험

-----

## 8\. 감사의 글

\*본 데이터 획득 시스템(DAQ)의 개발을 위해 하드웨어 연동 로우레벨 라이브러리(nfadc400, 6uvme)를 제공해주시고 아낌없는 기술 지원을 해주신 Notice Korea 김상열 사장님께 깊은 감사를 표합니다.

\*데이터 분석 코어에 사용된 오픈소스 프레임워크인 CERN ROOT 개발진과, 대시보드 UI 및 초고속 통신 아키텍처 구현에 사용된 PyQt5, ZeroMQ 커뮤니티의 기술적 기여를 인정합니다.

\*무엇보다 본 프레임워크를 비롯한 기초 과학 연구의 밑바탕에는 국민 여러분께서 땀 흘려 조성해 주신 소중한 연구기금이 있습니다. 대한민국 과학 기술의 진보와 기초 학문의 발전을 위해 묵묵히, 그리고 아낌없는 지지와 성원을 보내주신 국민 여러분께 가장 깊고 진심 어린 감사의 말씀을 올립니다.




