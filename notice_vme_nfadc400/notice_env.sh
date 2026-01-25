#!/bin/bash
# notice_env.sh : Notice Korea Module Environment Setup
# Usage: source notice_env.sh

# [1] NKHOME 자동 설정 (스크립트 위치 기준)
# --------------------------------------------------------------------------
# BASH_SOURCE를 사용하여 이 스크립트가 있는 디렉토리를 절대 경로로 구합니다.
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# NKHOME이 설정되어 있지 않거나, 현재 설정된 경로가 유효하지 않으면 재설정합니다.
if [ -z "$NKHOME" ] || [ ! -d "$NKHOME" ]; then
    export NKHOME="$SCRIPT_DIR"
fi

echo ">> [Notice Env] Setting up environment..."
echo "   * NKHOME : $NKHOME"

# 경로 유효성 검사
if [ ! -d "$NKHOME" ]; then
    echo "   [ERROR] NKHOME directory does not exist! Please check installation."
    return 1 2>/dev/null || exit 1
fi

# [2] LibUSB 설정 (Rocky Linux 9 / RHEL 9 표준 경로)
# --------------------------------------------------------------------------
export LIBUSB_INC=/usr/include/libusb-1.0
export LIBUSB_LIB=/usr/lib64

if [ ! -d "$LIBUSB_INC" ]; then
    echo "   [WARNING] libusb header directory not found at $LIBUSB_INC"
fi

# [3] ROOT (CERN) 설정
# --------------------------------------------------------------------------
# ROOTSYS가 설정되어 있지 않으면 찾습니다.
if [ -z "$ROOTSYS" ]; then
    if [ -f "/cern/root/bin/thisroot.sh" ]; then
        source /cern/root/bin/thisroot.sh
    elif command -v root-config &> /dev/null; then
        export ROOTSYS=$(root-config --prefix)
    else
        echo "   [WARNING] ROOT installation not found. 'root-config' command is missing."
    fi
fi
echo "   * ROOTSYS: ${ROOTSYS:-"Not Found"}"

# [4] PATH 및 Library Path 설정 (중복 방지 적용)
# --------------------------------------------------------------------------
# bin 경로 추가
if [[ ":$PATH:" != *":$NKHOME/bin:"* ]]; then
    export PATH=$NKHOME/bin:$PATH
fi

# lib 경로 추가
if [[ ":$LD_LIBRARY_PATH:" != *":$NKHOME/lib:"* ]]; then
    export LD_LIBRARY_PATH=$NKHOME/lib:$LD_LIBRARY_PATH
fi

# 컴파일 편의를 위한 Include Path
export CPLUS_INCLUDE_PATH=$NKHOME/include:$CPLUS_INCLUDE_PATH

echo ">> [Notice Env] Setup Complete."

echo "======================================================"
echo " Notice Korea Environment Loaded (Rocky Linux 9)"
echo "   * NKHOME  : $NKHOME"
echo "   * ROOTSYS : $ROOTSYS"
echo "   * LibUSB  : $LIBUSB_INC"
echo "======================================================"