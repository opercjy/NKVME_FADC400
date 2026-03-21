#!/bin/bash
# ====================================================================
# NKVME_FADC400 Environment Setup Script
# Usage: source setup.sh
# ====================================================================

# 현재 스크립트가 있는 디렉토리를 절대 경로로 추출
export NKVME_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# 1. 실행 파일(bin)을 시스템 PATH에 등록
# (이제 터미널 어디서든 daq_gui.py 나 frontend_nfadc400 을 바로 칠 수 있습니다)
export PATH=$NKVME_ROOT/bin:$PATH

# 2. 공유 라이브러리(lib)를 시스템 LD_LIBRARY_PATH에 등록
# (ROOT나 시스템이 libNKVME_FADC400.so를 자동으로 찾게 됩니다)
export LD_LIBRARY_PATH=$NKVME_ROOT/lib:$LD_LIBRARY_PATH

# 3. 객체 헤더(include)를 ROOT_INCLUDE_PATH에 등록
# (오프라인 매크로 작성 시 경로 없이 #include "Pmt.hh" 만 써도 인식됩니다)
export ROOT_INCLUDE_PATH=$NKVME_ROOT/objects/include:$ROOT_INCLUDE_PATH

echo "=========================================================="
echo " ✅ NKVME_FADC400 Environment Successfully Loaded!"
echo "   - 📂 Project Root : $NKVME_ROOT"
echo "   - ⚙️ PATH updated"
echo "   - 📚 LD_LIBRARY_PATH updated (libNKVME_FADC400.so)"
echo "   - 🧠 ROOT_INCLUDE_PATH updated"
echo "=========================================================="
