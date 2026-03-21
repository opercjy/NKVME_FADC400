```markdown
# NoticeDAQ : Advanced VME FADC400 & DCS Control Framework

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![ROOT](https://img.shields.io/badge/ROOT-6.00%2B-red.svg)
![Python](https://img.shields.io/badge/Python-3.8%2B-yellow.svg)
![PyQt5](https://img.shields.io/badge/PyQt5-GUI-brightgreen.svg)

NoticeDAQ는 **Notice Korea 6UVME 및 NFADC400 모듈**을 제어하기 위한 고성능 C++17 / ROOT 6 기반 데이터 획득(DAQ) 시스템이자, **CAEN 고전압 장비(HV)** 제어 및 자동화 시퀀서를 완벽하게 통합한 입자물리 실험실용 **All-in-One Slow Control System**입니다.

---

## 🌟 Key Features (주요 기능)

### 1. Zero Dead-Time DAQ Core (C++)
- **Ping-Pong Multi-threading**: 하드웨어 하프 버퍼를 폴링하는 생산자(Producer) 스레드와 디스크/네트워크 I/O를 담당하는 소비자(Consumer) 스레드를 완벽히 분리하여 수집 데드타임(Dead-time)을 0으로 수렴시켰습니다.
- **Absolute Memory Safety**: `TClonesArray`와 `ConstructedAt`을 활용한 **Object Pool** 아키텍처를 도입하여, 고속 수집 시 발생하는 메모리 파편화 및 힙(Heap) 누수를 원천 차단했습니다.

### 2. Central Control Dashboard (PyQt5)
- **Auto-Sequencer**: 문턱값(Threshold)을 자동으로 변경하며 스캔하는 **[THR Scan]** 기능과, 디스크 안전을 위해 지정된 시간(또는 이벤트) 단위로 파일을 자동 분할하는 **[Long Run Chunking]** 기능을 내장했습니다.
- **Auto-Increment Run Number**: 이전 수집 기록을 분석하여 다음 Run 번호를 자동으로 추천하고 기입해 줍니다.
- **E-Logbook**: 수집이 종료될 때마다 SQLite 데이터베이스(`daq_history.db`)에 메타데이터가 영구 기록되며, 대시보드에서 직접 코멘트를 수정하고 저장할 수 있습니다.

### 3. Config & Path Manager
- **Dynamic Routing**: 생성되는 대용량 데이터(`.root`)의 저장 경로를 외부 스토리지나 NAS 등 원하는 폴더로 실시간 변경할 수 있습니다.
- **Built-in Editor**: 터미널을 열 필요 없이 GUI 내부에서 직접 하드웨어 파라미터 파일(`*.config`)을 수정하고 다른 이름으로 저장할 수 있습니다.

### 4. Hardware DCS (Detector Control System)
- **CAEN Integration**: `caen-libs` 패키지를 통해 CAEN SY4527 등 메인프레임과 통신하여, 장착된 보드와 채널을 자동으로 인식해 동적 제어 UI를 생성합니다. 전압 인가 및 실시간(VMon, IMon) 모니터링이 가능합니다.
- **Analog Fallback Mode**: 아날로그 수동 장비 사용 시 체크박스를 해제하면 통신 폴링이 안전하게 차단되는 예외 처리(Fault-Tolerant)가 적용되어 있습니다.

### 5. Educational TLU Simulator
- **Trigger Logic Visualizer**: 초보 연구원을 위해 직관적인 ASCII 전자 회로도(Schematic Art)를 제공합니다. 드롭다운에서 원하는 동시계수(Coincidence) 물리 논리를 선택하면, FPGA 내부 진리표(Truth Table) 연산을 통해 설정 파일에 기입할 정확한 16진수 TLT 코드(ex: `0xFFFE`)를 자동으로 계산해 줍니다.

---

## 📁 Project Structure

```text
NoticeDAQ/
├── bin/                  # 빌드된 C++ 실행 파일 및 배포된 Python GUI 스크립트
├── config/               # DAQ 하드웨어 구동 파라미터 파일 (*.config)
├── rules/                # Linux USB-VME 권한 설정 UDEV 룰
│
├── gui/                  # [Python] 통제실 GUI 및 모듈
│   ├── daq_gui.py        # 메인 대시보드 및 자동화 시퀀서
│   ├── hv_control.py     # CAEN 고전압 제어 및 모니터링 패널
│   ├── tlu_simulator.py  # 트리거 논리 시뮬레이터 (회로도 시각화)
│   └── config_manager.py # 설정 파일 에디터 및 데이터 저장 경로 관리자
│
├── objects/              # [C++] ROOT 직렬화 데이터 객체 (RawData, Pmt 등)
├── frontend/             # [C++] DAQ 멀티스레드 코어 및 오프라인 분석기
└── display/              # [C++] 실시간 논블로킹 파형 모니터 (OnlineMonitor)
```

---

## 🚀 Build & Run Guide

### 1. 환경 설정 및 USB 권한 인가
처음 1회에 한하여 USB-VME 통신 권한을 시스템에 등록하고, ROOT 및 라이브러리 경로를 불러옵니다.
```bash
cd rules
sudo ./setup_usb.sh
cd ..
source setup.sh
```

### 2. 프로젝트 통합 빌드 (CMake)
멀티 보드 확장을 대비하여 타겟명에 모델명(`_nfadc400`)이 붙습니다. 빌드 시 `gui/` 폴더의 파이썬 스크립트들이 자동으로 `bin/` 폴더로 복사되며 실행 권한이 부여됩니다.
```bash
mkdir build && cd build
cmake .. 
make -j4
```

### 3. 통합 통제실 GUI 가동 (추천)
모든 조작(경로 설정, 데이터 수집, HV 제어, 런 기록)은 이 대시보드 하나에서 완벽하게 통제됩니다. 파이썬 가상환경(venv 등)이 있다면 활성화한 후 실행하십시오.
```bash
cd ..
./bin/daq_gui.py
```

### 4. 수동 CLI 모드 (옵션)
GUI 없이 백그라운드 환경이나 bash 스크립트로 직접 구동할 경우 아래 명령어를 사용합니다.
```bash
# 실시간 파형 모니터 백그라운드 띄우기
./bin/OnlineMonitor_nfadc400 &

# DAQ 백엔드 코어 수동 실행 (설정 파일, 출력 파일, 획득 이벤트 수 지정)
./bin/frontend_nfadc400 -f ./config/ch1.config -o data/run_101.root -n 5000
```

```

