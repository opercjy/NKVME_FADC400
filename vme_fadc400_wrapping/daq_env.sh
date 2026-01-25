#!/bin/bash
# daq_env.sh : DAQ Environment Setup Script
# Usage: source daq_env.sh

echo "======================================================"
echo " DAQ Environment Setup (Rocky Linux 9 / ROOT 6)"
echo "======================================================"

# [1] DAQHOME 자동 설정 (스크립트 위치 기준)
# --------------------------------------------------------------------------
# 이 스크립트가 있는 곳을 DAQHOME으로 설정합니다.
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
export DAQHOME="$SCRIPT_DIR"

if [ ! -d "$DAQHOME" ]; then
    echo " [ERROR] DAQHOME could not be determined."
    return 1 2>/dev/null || exit 1
fi
echo " * DAQHOME : $DAQHOME"

# [2] Notice Module (NKHOME) 경로 탐색
# --------------------------------------------------------------------------
# NKHOME이 없으면 형제 디렉토리에서 'notice_vme_nfadc400'을 찾습니다.
if [ -z "$NKHOME" ]; then
    # 1순위: DAQHOME 옆에 있는 notice_vme_nfadc400
    POSSIBLE_NKHOME=$(cd "$DAQHOME/../notice_vme_nfadc400" 2>/dev/null && pwd)
    
    if [ -n "$POSSIBLE_NKHOME" ] && [ -d "$POSSIBLE_NKHOME" ]; then
        export NKHOME="$POSSIBLE_NKHOME"
    else
        # 2순위: 시스템 기본 경로 (fallback)
        export NKHOME=/usr/local/notice
    fi
fi

# NKHOME 유효성 검사 및 로드
if [ -f "$NKHOME/notice_env.sh" ]; then
    echo " * NKHOME  : $NKHOME (Found)"
    source "$NKHOME/notice_env.sh"
else
    echo " [ERROR] Cannot find 'notice_env.sh' in $NKHOME"
    echo "         Please check if 'notice_vme_nfadc400' is installed correctly."
    return 1 2>/dev/null || exit 1
fi

# [3] 라이브러리 및 실행 경로 추가
# --------------------------------------------------------
# [Library] Objects 라이브러리 경로 추가
if [[ ":$LD_LIBRARY_PATH:" != *":$DAQHOME/objects/lib:"* ]]; then
    export LD_LIBRARY_PATH=$DAQHOME/objects/lib:$LD_LIBRARY_PATH
fi

# [Executable] bin 폴더를 PATH에 추가
if [[ ":$PATH:" != *":$DAQHOME/bin:"* ]]; then
    export PATH=$DAQHOME/bin:$PATH
fi

# [Include] 컴파일 편의를 위해 Include Path 추가
# 기존 값에 중복되지 않게 추가하는 것이 좋으나, 간단하게 앞에 붙입니다.
export CPLUS_INCLUDE_PATH=$DAQHOME/objects/include:$NKHOME/include:$CPLUS_INCLUDE_PATH

# [4] 최종 확인
# --------------------------------------------------------
echo " * BIN_PATH: $DAQHOME/bin"
echo " * ROOTSYS : ${ROOTSYS:-"Not Found"}"
echo "======================================================"
echo " Environment Loaded Successfully."
echo " You can now run: frontend_nfadc400 -f configs/daq.config"
echo "======================================================"

echo "======================================================"
echo " DAQ Environment Loaded (Rocky Linux 9 / ROOT 6)"
echo "   * DAQHOME : $DAQHOME"
echo "   * BIN_PATH: $DAQHOME/bin"
echo "   * NKHOME  : $NKHOME"
echo "   * ROOTSYS : $ROOTSYS"
echo "======================================================"