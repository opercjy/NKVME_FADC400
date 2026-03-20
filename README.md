
# NoticeDAQ : Advanced VME FADC400 & DCS Control Framework

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![ROOT](https://img.shields.io/badge/ROOT-6.00%2B-red.svg)
![Python](https://img.shields.io/badge/Python-3.8%2B-yellow.svg)
![PyQt5](https://img.shields.io/badge/PyQt5-GUI-brightgreen.svg)

NoticeDAQ는 **Notice Korea 6UVME 및 NFADC400 모듈**을 제어하기 위한 고성능 C++17 / ROOT 6 기반 데이터 획득(DAQ) 시스템이자, **CAEN / ORTEC 고전압 장비(HV)** 제어 및 자동화 시퀀서를 통합한 입자물리 실험실용 **All-in-One Slow Control System**입니다.

---

##  Key Features (주요 기능)

### 1.  Zero Dead-Time DAQ Core (C++)
- **Ping-Pong Multi-threading**: 하드웨어 하프 버퍼를 폴링하는 생산자(Producer) 스레드와 디스크/네트워크 I/O를 담당하는 소비자(Consumer) 스레드를 완벽히 분리하여 수집 데드타임(Dead-time)을 0으로 수렴시켰습니다.
- **Absolute Memory Safety**: `TClonesArray`와 `ConstructedAt`을 활용한 **Object Pool** 아키텍처를 도입하여, 고속 수집 시 발생하는 메모리 파편화 및 힙(Heap) 누수를 원천 차단했습니다.

### 2.  Central Control Dashboard (PyQt5)
- **Auto-Sequencer**: 문턱값(Threshold)을 자동으로 변경하며 데이터를 수집하는 **[THR Scan]** 기능과, 장기 수집 시 디스크 안전을 위해 시간 단위로 파일을 자동 분할하는 **[Long Run Chunking]** 기능을 내장했습니다.
- **E-Logbook**: 수집을 시작하고 종료할 때마다 SQLite 데이터베이스(`daq_history.db`)에 런(Run) 메타데이터와 상태가 자동으로 영구 기록됩니다.
- **Live Status Parsing**: C++ 백엔드의 출력을 정규식(Regex)으로 낚아채어, 이벤트 카운터(`QLCDNumber`)와 진행률 바(`QProgressBar`)로 직관적으로 시각화합니다.

### 3.  Hardware DCS (Detector Control System)
- **CAEN & ORTEC Integration**: `gui/hv_control.py` 모듈을 통해 CAEN SY4527 메인프레임 및 ORTEC 장비의 고전압(HV)을 GUI에서 직접 인가하고 실시간(VMon, IMon)으로 모니터링합니다.
- **Analog Fallback Mode**: 아날로그 장비 사용 시 SCM(Slow Control Module) 체크박스를 해제하면 통신 타이머가 안전하게 차단되는 예외 처리(Fault-Tolerant)가 적용되어 있습니다.

### 4.  Educational TLU Simulator
- **Trigger Logic Visualizer**: 초보 연구원을 위해 `gui/tlu_simulator.py`를 탑재했습니다. GUI에서 채널과 AND/OR 논리 게이트를 클릭하면 직관적인 ASCII 회로도와 함께 설정 파일(`config`)에 기입할 정확한 16진수 TLT 코드(ex: `0xFFFE`)를 자동으로 계산해 줍니다.

---

##  Project Structure

```text
NoticeDAQ/
├── bin/                  # 빌드된 C++ 실행 파일 및 배포된 Python GUI
├── config/               # DAQ 하드웨어 구동 파라미터 파일 (*.config)
├── rules/                # Linux USB-VME 권한 설정 UDEV 룰
│
├── gui/                  # [Python] 통제실 GUI 및 모듈
│   ├── daq_gui.py        # 메인 대시보드 및 자동화 시퀀서
│   ├── hv_control.py     # CAEN/ORTEC 고전압 제어 패널 모듈
│   └── tlu_simulator.py  # 트리거 논리 시뮬레이터 모듈
│
├── objects/              # [C++] ROOT 직렬화 데이터 객체 (RawData, Pmt 등)
├── frontend/             # [C++] DAQ 멀티스레드 코어 및 오프라인 분석기
└── display/              # [C++] 실시간 논블로킹 파형 모니터 (OnlineMonitor)
```

---

##  Build & Run Guide

### 1. 환경 설정 및 USB 권한 인가
처음 1회에 한하여 USB-VME 통신 권한을 시스템에 등록합니다.
```bash
$cd rules && sudo ./setup_usb.sh$ cd ..
$ source setup.sh
```

### 2. 프로젝트 통합 빌드 (CMake)
빌드 시 `gui/` 폴더의 파이썬 스크립트들이 자동으로 `bin/` 폴더로 복사되며 실행 권한이 부여됩니다.
```bash
$ mkdir build && cd build
$ cmake .. && make -j4
```

### 3. 통합 통제실 GUI 가동 (추천)
모든 조작(설정, 수집, HV 제어, 오프라인 분석)은 이 대시보드 하나에서 이루어집니다.
```bash
$cd ..$ ./bin/daq_gui.py
```

### 4. 수동 CLI 모드 (옵션)
GUI 없이 백그라운드 환경이나 스크립트로 직접 구동할 경우:
```bash
# 실시간 파형 모니터 백그라운드 실행
$ ./bin/OnlineMonitor &

# DAQ 수동 실행 (설정 파일 및 출력 파일 지정)
$ ./bin/frontend -f ./config/ch1.config -o data/run_101.root -n 5000
```
