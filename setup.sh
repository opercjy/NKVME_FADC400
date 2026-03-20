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
