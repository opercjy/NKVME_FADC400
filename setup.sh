#!/bin/bash
# ====================================================================
# NKVME_FADC400 & Notice Hardware Environment Setup Script
# Usage: source setup.sh
# ====================================================================

# 1. 프로젝트 최상단 디렉토리 자동 인식
export NKVME_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"

# 2. Notice Korea 하드웨어 라이브러리 (NKHOME) 및 USB 설정
export NKHOME=/usr/local/notice
#export LIBUSB_INC=/usr/include/libusb-1.0
# [참고] RHEL/CentOS/Rocky 계열은 /usr/lib64, Ubuntu/Debian 계열은 /usr/lib/x86_64-linux-gnu 사용
export LIBUSB_LIB=/usr/lib64

# 3. CERN ROOT 환경 변수 로드 (thisroot.sh)
# 이미 ROOTSYS가 잡혀있지 않은 경우에만 실행하도록 처리하여 중복 로드 방지
if [ -z "$ROOTSYS" ]; then
  echo "[INFO] ROOT environment not found. Searching for thisroot.sh..."

  # 주로 사용되는 ROOT 설치 경로들을 순차적으로 탐색하여 로드
  if [ -f "/usr/local/bin/thisroot.sh" ]; then
    source /usr/local/bin/thisroot.sh
  elif [ -f "/home/notice/root/bin/thisroot.sh" ]; then
    source /home/notice/root/bin/thisroot.sh
  elif [ -f "/opt/root/bin/thisroot.sh" ]; then
    source /opt/root/bin/thisroot.sh
  else
    echo "[WARNING] thisroot.sh not found! Please check your ROOT installation path."
  fi
fi

# 4. PATH 설정 (응용 프로그램 bin 및 벤더 bin 폴더 동시 등록)
if [ -z "${PATH}" ]; then
  export PATH=$NKVME_ROOT/bin:$NKHOME/bin
else
  export PATH=$NKVME_ROOT/bin:$NKHOME/bin:$PATH
fi

# 5. 공유 라이브러리(LD_LIBRARY_PATH) 설정
# (오프라인 분석용 libNKVME_FADC400.so 및 Notice 하드웨어 라이브러리 동시 등록)
if [ -z "${LD_LIBRARY_PATH}" ]; then
  export LD_LIBRARY_PATH=$NKVME_ROOT/lib:$NKHOME/lib:/usr/local/lib
else
  export LD_LIBRARY_PATH=$NKVME_ROOT/lib:$NKHOME/lib:/usr/local/lib:$LD_LIBRARY_PATH
fi

# 6. ROOT C++ 헤더 인식 경로 (오프라인 매크로에서 #include "Pmt.hh" 등을 바로 쓰기 위함)
if [ -z "${ROOT_INCLUDE_PATH}" ]; then
  export ROOT_INCLUDE_PATH=$NKVME_ROOT/objects/include
else
  export ROOT_INCLUDE_PATH=$NKVME_ROOT/objects/include:$ROOT_INCLUDE_PATH
fi

echo "=========================================================="
echo " ✅ NKVME_FADC400 & Notice Hardware Environment Loaded!"
echo "   - 📂 Project Root : $NKVME_ROOT"
echo "   - ⚙️ NKHOME       : $NKHOME"
echo "   - 📚 ROOTSYS      : $ROOTSYS"
echo "   - 🛠️ PATH updated (Added Project & Notice bin)"
echo "   - 🔗 LD_LIBRARY_PATH updated (libNKVME_FADC400.so & Notice lib)"
echo "   - 🧠 ROOT_INCLUDE_PATH updated (Objects Headers)"
echo "=========================================================="

# FADC400 실행 파일들을 어디서든 실행할 수 있도록 시스템 PATH에 등록
export PATH="$(pwd)/bin:$PATH"