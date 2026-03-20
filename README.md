
# NoticeDAQ (VME FADC400 Data Acquisition System)

Notice Korea 6UVME 및 NFADC400 모듈을 제어하기 위한 고성능 C++17 / ROOT 6 기반 DAQ 프레임워크입니다.

##  Key Features
- **Zero Dead-time DAQ**: 하프 버퍼(Ping-Pong) 병렬 판독 및 Multi-threading (Producer-Consumer) 아키텍처 적용
- **Memory Safety**: `TClonesArray` 와 `ConstructedAt`을 활용한 객체 풀링(Object Pool)으로 메모리 단편화 및 누수(Memory Leak) 완벽 차단
- **Robust Configuration**: 쌍따옴표 스트리핑, OOM 방어 한계치 검사, Safe-Ignore 패턴이 적용된 무결점 파서
- **Real-time Monitoring**: 비동기 소켓(`select()`) 기반의 Non-blocking 실시간 4채널 파형 GUI 뷰어 (TCP Backpressure 방어 적용)
- **Offline Analyzer**: 터미널 입력(`stdin`) 비동기 처리가 적용된 대화형 데이터 분석 파이프라인 및 부동소수점 예외(NaN) 방어
- **GUI Control Center**: PyQt5 및 SQLite 기반의 중앙 제어 및 E-Logbook 시스템 내장



##  Environment Setup (`setup.sh`)

컴파일 및 실행 전, 하드웨어 통신 라이브러리와 ROOT 환경 변수를 설정해야 합니다. 프로젝트 최상단에 아래 내용으로 `setup.sh` 스크립트를 생성하고 실행하십시오.

```bash
#!/bin/bash

# 1. Notice Korea 하드웨어 라이브러리 경로 설정
export NKHOME=/usr/local/notice

# 2. ROOT 6 환경 설정 (이미 .bashrc 등에 설정되어 있다면 생략 가능)
if [ -z "$ROOTSYS" ]; then
    echo "Loading ROOT environment..."
    # 사용자 환경에 맞는 ROOT thisroot.sh 경로로 수정하세요
    source /usr/local/bin/thisroot.sh
fi

# 3. 런타임에 동적 라이브러리를 찾을 수 있도록 PATH 설정
export DAQ_PROJECT_DIR=$(pwd)
export LD_LIBRARY_PATH=${NKHOME}/lib:${LD_LIBRARY_PATH}

echo "======================================================"
echo " NoticeDAQ Environment Setup Completed."
echo "  * NKHOME          : $NKHOME"
echo "  * ROOTSYS         : $ROOTSYS"
echo "  * DAQ_PROJECT_DIR : $DAQ_PROJECT_DIR"
echo "======================================================"
```

---

##  Build & Run

### 1. 환경 설정 적용 및 USB 권한 세팅
```bash
$ cd rules && sudo ./setup_usb.sh
$ cd ..
$ source setup.sh
```

### 2. 프로젝트 빌드 (Modern CMake)
```bash
$ mkdir build && cd build
$ cmake .. && make -j4
```

### 3. 통합 GUI 컨트롤 센터 실행 (추천)
PyQt5 기반의 중앙 제어 시스템을 통해 설정, 수집, 모니터링, 오프라인 분석을 한 번에 제어할 수 있습니다.
```bash
$ cd ..
$ ./bin/daq_gui.py
```

### 4. 수동 CLI 실행 (옵션)
GUI 없이 백그라운드나 터미널에서 직접 실행할 경우:
```bash
# 실시간 모니터 백그라운드 실행
$ ./bin/OnlineMonitor &

# 1-Channel 테스트 모드로 DAQ 수집 시작
$ ./bin/frontend -f ./config/ch1.config -o run_101.root
```

