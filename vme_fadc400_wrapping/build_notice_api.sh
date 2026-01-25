#!/bin/bash
# build_notice_api.sh : DAQ Frontend & Production Build Script
# Usage: ./build_notice_api.sh

# 에러 발생 시 즉시 중단 (디버깅 용이)
set -e 

echo "================================================================================"
echo " DAQ Application (API) Build Start"
echo "================================================================================"

# 1. 환경 설정 로드 및 확인
# --------------------------------------------------------
if [ -z "$DAQHOME" ]; then
    if [ -f "./daq_env.sh" ]; then 
        source ./daq_env.sh
    else 
        echo " [ERROR] daq_env.sh not found. Run 'source daq_env.sh' first."
        exit 1
    fi
fi

# [함수] 청소 (Object 파일 삭제)
cleanup_objects() {
    echo "  -> Cleaning up intermediate object files..."
    rm -f *.o
    rm -f tmp/*.o
}

# --------------------------------------------------------
# 2. Objects Library Build (데이터 클래스)
# --------------------------------------------------------
echo -e "\n>>> [1/2] Building Data Objects (ROOT Library)..."

if [ -d "$DAQHOME/objects" ]; then
    cd $DAQHOME/objects
    make clean > /dev/null
    make
    
    # [검증] 라이브러리 및 PCM 파일 생성 확인
    if [ ! -f "lib/libobjects.so" ]; then
        echo " [ERROR] libobjects.so creation failed!"
        exit 1
    fi
    
    # ROOT 6용 PCM 파일 확인 (매우 중요)
    PCM_COUNT=$(ls lib/*.pcm 2>/dev/null | wc -l)
    if [ "$PCM_COUNT" -eq 0 ]; then
        echo " [WARNING] No .pcm files found in objects/lib/. ROOT 6 might fail to read objects."
    fi

    # 빌드 성공 시 Object 파일 삭제
    cleanup_objects
    
    echo " -> Data Objects built successfully."
else
    echo " [ERROR] 'objects' directory not found!"
    exit 1
fi

# --------------------------------------------------------
# 3. Frontend & Production Build (실행 파일)
# --------------------------------------------------------
echo -e "\n>>> [2/2] Building DAQ & Production Apps..."

# bin 디렉토리 준비
mkdir -p $DAQHOME/bin

if [ -d "$DAQHOME/frontend" ]; then
    cd $DAQHOME/frontend
    make clean > /dev/null
    make
    
    # 설치 (bin 폴더로 복사)
    if [ -f "frontend_nfadc400" ]; then
        cp frontend_nfadc400 $DAQHOME/bin/
        echo "  -> Installed 'frontend_nfadc400' to bin/"
    else
        echo " [ERROR] frontend_nfadc400 build failed!"
        exit 1
    fi

    if [ -f "production_nfadc400" ]; then
        cp production_nfadc400 $DAQHOME/bin/
        echo "  -> Installed 'production_nfadc400' to bin/"
    else
        echo " [ERROR] production_nfadc400 build failed!"
        exit 1
    fi
    
    # 빌드 성공 시 Object 파일 삭제
    cleanup_objects
else
    echo " [ERROR] 'frontend' directory not found!"
    exit 1
fi

echo -e "\n================================================================================"
echo " API Build Summary"
echo "================================================================================"
if [ -f "$DAQHOME/objects/lib/libobjects.so" ]; then
    echo "[OK] Objects Lib   : Built ($DAQHOME/objects/lib/libobjects.so)"
else
    echo "[!!] Objects Lib   : FAILED"
fi

if [ -f "$DAQHOME/bin/frontend_nfadc400" ]; then
    echo "[OK] DAQ Frontend  : Installed (frontend_nfadc400)"
else
    echo "[!!] DAQ Frontend  : FAILED"
fi

if [ -f "$DAQHOME/bin/production_nfadc400" ]; then
    echo "[OK] Production    : Installed (production_nfadc400)"
else
    echo "[!!] Production    : FAILED"
fi
echo "================================================================================"
echo " You can now run:"
echo "   frontend_nfadc400 -f configs/daq.config"
echo "================================================================================"