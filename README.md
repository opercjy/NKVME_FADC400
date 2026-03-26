-----

# NKVME\_FADC400 : NoticeDAQ Central Control
![C++](https://img.shields.io/badge/C++-17-blue?style=flat-square&logo=c%2B%2B)
![Python](https://img.shields.io/badge/Python-3.x-3776AB?style=flat-square&logo=python&logoColor=white)
![ROOT](https://img.shields.io/badge/Framework-CERN%20ROOT%206-005aaa?style=flat-square)
![PyQt5](https://img.shields.io/badge/GUI-PyQt5-41CD52?style=flat-square&logo=qt&logoColor=white)
![CMake](https://img.shields.io/badge/Build-CMake-064F8C?style=flat-square&logo=cmake&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Linux-FCC624?style=flat-square&logo=linux&logoColor=black)
![Status](https://img.shields.io/badge/Status-Active-brightgreen?style=flat-square)
![License](https://img.shields.io/badge/License-Notice_Authorized-red?style=flat-square)

> **Notice & License**
> 본 레포지토리는 하드웨어 제조사인 Notice Korea의 공식적인 동의 및 허가 하에 로우레벨 통신 라이브러리(nfadc400, 6uvme)의 소스 코드를 공유하며, 연구 목적의 프레임워크 통합 및 지속적인 유지보수를 수행하고 있습니다.


**NKVME\_FADC400**은 VME 기반 Notice FADC400 (400MS/s, 12-bit, 2.5ns/sample) 보드를 제어하고 데이터를 수집, 모니터링, 오프라인 분석을 수행하는 통합 데이터 획득(DAQ) 프레임워크입니다.

본 시스템은 데이터 처리는 C++ ROOT 백엔드에서, 사용자 인터페이스와 DB 제어는 Python PyQt5 프론트엔드에서 수행하는 하이브리드 아키텍처로 설계되어 안정성과 유지보수 편의성을 제공합니다. 특히 극한의 노이즈 환경과 고속 트리거 획득 상황에서도 VME 버스 병목으로 인해 시스템이 멈추지 않도록, 하드웨어 핑퐁 버퍼링(Ping-Pong Buffering) 논리를 완벽히 복원하고 데이터 파싱 오버헤드를 극단적으로 제거한 **제로 카피(Zero-Copy) 엔진**이 탑재되어 있습니다.

-----

## 1\. 시스템 아키텍처 (System Architecture)

본 프레임워크는 4개의 논리적 계층으로 구성되어 있습니다.

1.  **Hardware / Vendor Layer (`nfadc400`, `6uvme`, `display`):** Notice Korea에서 제공한 USB-VME 브릿지, FADC400 레지스터 제어 및 VME 버스 상태 모니터링용 C/C++ 로우레벨 통신 라이브러리.
2.  **C++ DAQ Core Layer (`frontend` & `objects`):** 하드웨어를 제어하고, 병렬 스레드(Producer-Consumer)로 디스크(ROOT 파일) 기록 및 TCP 소켓으로 브로드캐스팅하는 수집 엔진. VME 버스의 I/O 대역폭 한계를 돌파하기 위한 무결성 방어 로직이 적용되어 있습니다.
3.  **C++ DQM Layer (`display`):** 프론트엔드 데이터를 수신하여 실시간으로 물리량(Baseline, Charge)을 환산하고 캔버스에 렌더링하는 비동기 렌더링 모니터링 모듈.
4.  **Python Master UI Layer (`gui`):** C++ 백엔드 프로세스의 라이프사이클을 통제하고, SQLite DB, HV, TLU 제어 등을 관리하는 중앙 대시보드.

-----

## 2\. 핵심 기능 및 상세 스펙 (Core Features)

### 2.1 C++ DAQ 백엔드 (`frontend_nfadc400`) 핵심 최적화

  * **Zero-Copy 벌크 전송 (Bulk Transfer):** 기존 구조에서 이벤트 단위로 파싱하던 오버헤드를 완전히 제거했습니다. VME 버스에서 덤프해온 1MB 단위의 순수 바이너리 블록을 단일 ROOT 객체 배열에 다이렉트로 매핑하여 직렬화(Serialization)합니다. 이로써 CPU 부하를 0%에 가깝게 유지하며 VME-USB 통신이 허락하는 물리적 한계점까지 수집 속도를 극대화했습니다.
  * **완벽한 Ping-Pong Arming:** 하드웨어의 이중 버퍼(Buffer 0, Buffer 1) 교차 수집 논리를 복원하여, PC가 VME 버스를 통해 데이터를 읽어가는 동안에도 FPGA가 멈춤 없이(Zero Dead-time) 다음 이벤트를 병렬 수집할 수 있도록 아키텍처를 재설계했습니다.
  * **하드웨어 락(Lock) 해제 및 페데스탈 자동 보정:** 파형 버퍼와 함께 태그(`dump_TAG`) 버퍼를 명시적으로 병행 추출하여 하드웨어의 락(Lock) 현상을 해제하였으며, 초기화 단계에서 페데스탈 측정(`measure_PED`)을 강제 수행하여 트리거 0Hz 멈춤 현상을 원천 차단했습니다.

### 2.2 실시간 DQM (Data Quality Monitor) 및 동적 렌더링

  * **비동기 화면 렌더링:** 초당 수백 개의 이벤트가 쏟아져도 데이터 수신(100%)과 GUI 화면 갱신(10FPS 제한)을 완전히 분리하여 시스템 프리징 현상을 제거했습니다.
  * **실시간 물리량 환산:** 수신된 벌크 데이터 블록의 최신 이벤트를 역산하여 실시간으로 Baseline을 계산하고 적분 전하량(Charge) 히스토그램을 누적합니다.
  * **Auto-Clear 및 추적 엔진:** 새로운 런(Run) 번호 변경 및 TCP 소켓 재연결 감지 시 그래프 자동 초기화 로직이 적용되었습니다.

### 2.3 Python PyQt5 대시보드 (GUI)

  * **실시간 대시보드 및 콘솔 덮어쓰기:** 수집된 이벤트 수, 진행 시간, 초당 트리거 처리율(Hz), 버퍼 큐 상태를 시각화하며, C++ 콘솔 출력을 줄바꿈(Newline) 없이 덮어쓰기(Overwrite) 방식으로 파싱하여 데이터 폭주 상황에서도 시각적 피로도를 극적으로 낮췄습니다.
  * **다크/라이트 테마 최적화:** 어두운 지하 실험실 환경을 고려하여 콘솔 창 가독성을 극대화한 ANSI 컬러 매핑을 적용했습니다.

### 2.4 오프라인 분석기 (`production_nfadc400`)

  * **순수 플랫 트리 (Pure Flat Tree) 변환:** VME 대역폭 극복을 위해 수집된 통짜 바이너리 블록을 오프라인에서 개별 이벤트 단위로 정밀 파싱합니다. 입자물리 데이터의 컷(Cut) 기반 분석에 최적화된 플랫 구조의 `*.root` 파일로 고속 변환합니다.

### 2.5 E-Logbook 데이터베이스 (`elog_manager.py`)

  * SQLite3 기반의 전자 로그북 모듈을 내장하여, 각 런(Run)의 품질(Quality), 총 이벤트 수, 처리율(Rate), 코멘트 등을 영구 보존합니다.

-----

## 3\. 디렉토리 구조 및 소스코드 명세 (Directory Details)

```text
NKVME_FADC400/
├── bin/                       # 빌드된 C++ 실행 파일 및 Python GUI 스크립트 모음
├── build/                     # CMake 빌드 디렉토리
├── config/                    # DAQ 하드웨어 설정 파일 모음 (ch1.config, ch8.config 등)
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
│   └── src/display/           # VME 상태창(NoticeDISPLAY) 모듈 (C 코어만 필수, ROOT 모듈은 레거시)
│
├── rules/                     # Linux 환경 USB 장치 권한 설정 스크립트 (setup_usb.sh)
├── setup.sh                   # 프로젝트 통합 환경 변수 셋업 스크립트
├── offline_spe.cpp            # 오프라인 ROOT 매크로: PMT SPE(단일 광전자) 교정용
└── offline_charge.cpp         # 오프라인 ROOT 매크로: 범용 전하량 스펙트럼 시각화
```

-----

## 4\. 설치 및 빌드 방법 (Build & Installation)

### 4.1 제조사 로우레벨 라이브러리 컴파일 (초보자 필독)

메인 프레임워크를 빌드하기 전에, 하드웨어 통신을 담당하는 제조사의 원본 라이브러리를 반드시 가장 먼저 컴파일해야 합니다.
*(주의: `display` 하위의 `displayroot` 모듈은 시스템에서 현재 사용하지 않는 레거시이므로 설치를 생략합니다.)*

```bash
# 1. 관리자 권한 획득 (su 또는 sudo su)
sudo su

# 2. 제조사 환경 변수 로드 (NKHOME 경로 설정 등)
source nfadc400/notice_env.sh

# 3. USB-VME 브릿지 통신 라이브러리 설치
cd nfadc400/src/6uvme/6uvme          
make clean; make; make install
cd ../6uvmeroot                      
make clean; make; make install

# 4. FADC400 제어 라이브러리 설치
cd ../../nfadc400/nfadc400           
make clean; make; make install
cd ../nfadc400root                   
make clean; make; make install

# 5. VME 상태창 (NoticeDISPLAY) 일반 C 라이브러리 설치
cd ../../display/display             
make clean; make; make install
# (참고: cd ../displayroot 빌드 과정은 미사용 레거시이므로 건너뜁니다.)

# 6. 관리자 모드 종료 및 최상단 디렉토리 복귀
exit
cd ../../../..
```

### 4.2 필수 패키지 설치

OS 환경에 맞게 시스템 의존성 패키지를 설치합니다. (CERN ROOT 6는 시스템에 사전 설치되어 있어야 합니다.)

**[Rocky Linux / AlmaLinux / CentOS]** - *권장 환경*

```bash
sudo dnf install epel-release
sudo dnf update
sudo dnf install libusb1-devel python3-pip python3-devel cmake gcc-c++ make sqlite
pip3 install PyQt5 caen-libs
```

### 4.3 하드웨어 USB 권한 설정 (최초 1회)

일반 사용자가 USB-VME 컨트롤러에 접근할 수 있도록 udev 룰을 등록합니다.

```bash
cd rules/
sudo ./setup_usb.sh
cd ..
```

### 4.4 메인 프로젝트 환경 변수 및 컴파일 (CMake)

라이브러리 셋업이 완료되었다면 메인 통합 프레임워크를 빌드합니다.

```bash
# 1. 메인 프로젝트 통합 환경 변수 로드
source setup.sh

# 2. CMake 빌드 수행
mkdir build && cd build
cmake ..
make -j4
```

> **[주의사항]**
> 터미널을 열어 DAQ를 구동할 때마다 반드시 프로젝트 최상단 폴더에서 `source nfadc400/notice_env.sh` 와 `source setup.sh` 두 명령어를 실행해야 정상 작동합니다.

-----

## 5\. 하드웨어 환경 설정 가이드 (Configuration)

FADC400의 트리거 한계를 올바르게 이끌어내기 위해 `config/*.config` 파일 수정 시 다음 사항을 반드시 준수해야 합니다.

  * **신호 극성 (POL):** PMT(광전자증폭관)에서 출력되는 신호는 Negative 펄스이므로 반드시 `POL 0` (4채널일 경우 `POL 0 0 0 0`)으로 설정해야 합니다. (`1`은 Positive)
  * **베이스라인 오프셋 (DACOFF):** Negative 펄스가 12-bit ADC(0\~4095) 대역 내에서 충분한 하강 공간을 가질 수 있도록, 베이스라인 오프셋을 높은 값(`DACOFF 3400`)으로 설정해야 합니다.
  * **트리거 동시계수 (TLT):** 모든 채널의 활성화를 허용하는 범용 설정값은 `TLT 0xFFFE` (Global OR) 입니다.

-----

## 6\. 오프라인 분석 매크로 (Offline Analysis Macros)

수집 및 오프라인 변환(`production_nfadc400`)이 완료된 `*_prod.root` 데이터는 순수 플랫 트리(Pure Flat Tree) 구조로 설계되어 있습니다. 이를 직관적으로 시각화하고 물리적 유의미성을 도출하기 위해 두 가지 핵심 ROOT 매크로를 제공합니다.

### 6.1 범용 전하량 스펙트럼 시각화 (`offline_charge.cpp`)

  * 변환된 데이터에서 각 채널별 적분 전하량(Charge) 스펙트럼을 Log-Y 스케일로 렌더링합니다.
  * 캡슐화된 `Pmt::Waveform()` 포인터를 직접 참조하여, 첫 번째 이벤트(Event 0)의 시계열 파형(Voltage Drop)을 시간(ns) 단위로 환산하여 우측 패널에 동시 표출합니다.
  * **실행:** `root -l 'offline_charge.cpp("data_prod.root")'`

### 6.2 PMT 단일 광전자 교정 (`offline_spe.cpp`)

  * PMT의 절대 이득(Absolute Gain)을 추출하기 위한 정밀 캘리브레이션 도구입니다.
  * **Multi-Gaussian 피팅:** Pedestal(전자 노이즈), 1st p.e.(단일 광전자), 2nd p.e.(이중 광전자) 피크를 합성 함수로 구성하여 스펙트럼을 해부합니다.
  * **Minuit2 최적화:** 기초적인 `Fit()` 함수의 수렴 실패를 방지하기 위해, 입자물리 표준인 `Minuit2` 미니마이저를 적용하고 파라미터의 물리적 제약 조건(Boundary Limits)을 부여하여 피팅 성공률과 정밀도를 극대화했습니다.
  * **실행:** `root -l 'offline_spe.cpp("data_prod.root")'`

-----

## 7\. 유지보수 가이드 (Maintenance)

  * **ROOT 딕셔너리:** 프로세스 간 통신용 공유 구조체(`objects/include/*.hh`)를 수정할 경우, 반드시 `build` 디렉토리에서 `make clean` 후 재빌드하여 딕셔너리를 갱신해야 합니다.
  * **제조사 라이브러리:** 펌웨어나 로우레벨 라이브러리 업데이트 시, `nfadc400/src/` 내부의 C/C++ 소스를 교체합니다. `frontend.cc` 내 하드웨어 초기화 시퀀스(레지스터 제어 순서)는 제조사 사양을 엄격히 반영한 것이므로 임의 변경을 지양합니다.

-----

## 8\. 그래픽 인터페이스 및 사용자 경험

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

## 9\. 감사의 글

본 데이터 획득 시스템(DAQ)의 완성은 보이지 않는 곳에서 헌신해 주신 많은 분들과 인프라 덕분입니다. 이에 깊은 감사를 표합니다.

**대한민국 국민 여러분께**
당장의 가시적인 이익이 보이지 않는 기초 과학의 길을 묵묵히 걸을 수 있는 것은, 국민 여러분께서 땀 흘려 조성해 주신 소중한 연구 기금 덕분입니다. 이 시스템이 앞으로 포착해 낼 우주의 미세한 신호들은 모두 국민 여러분의 지지와 성원이 만들어낸 결실입니다. 보내주신 아낌없는 믿음에 깊이 감사드리며, 그 무거운 책임감과 자긍심을 안고 연구의 최전선에서 쉼 없이 탐구하겠습니다.

**Notice Korea 김상열 박사님께**
뛰어난 하드웨어를 설계하고 연동 로우레벨 라이브러리(nfadc400, 6uvme)를 제공해 주신 Notice Korea 김상열 대표/박사님께 깊은 감사를 드립니다. 아낌없이 공유해 주신 로우 레벨 로직과 상세한 기술 지원은 이 프로젝트를 완성하는 가장 든든한 기반이 되었습니다.

**IBS 지하실험 연구단(CUP) 이재승 박사님께**
과거 전남대학교에서 현재 시스템의 핵심인 데이터 직렬화 및 원천 데이터 객체(Obj) 아키텍처의 초석을 설계해 주신 이재승 박사님께 각별한 감사의 말씀을 전합니다. 그 당시의 뛰어난 아키텍처 설계가 있었기에 현재의 극한 최적화가 가능했습니다.

데이터 분석 코어에 사용된 오픈소스 프레임워크인 CERN ROOT 개발진과, 대시보드 UI 구현에 사용된 PyQt5 커뮤니티의 기술적 기여에도 깊은 감사를 드립니다.






