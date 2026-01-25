#!/bin/bash
# install.sh : FADC400 Integrated Installer
# Updates: 
#  - Renamed build scripts (2025-01-26)
#  - Auto-load environment variables (Critical for "One-Click" install)

# 현재 디렉토리 저장 (절대 경로)
TOP_DIR=$(pwd)

echo "========================================================"
echo " FADC400 System Integration Installer"
echo "========================================================"

# ------------------------------------------------------
# 0. Environment Setup (Critical Step)
# ------------------------------------------------------
echo -e "\n>>> [Step 0] Setting up Build Environment..."

# DAQHOME과 NKHOME을 현재 위치 기준으로 강제 설정 (빌드 안전성 확보)
export DAQHOME=$TOP_DIR/vme_fadc400_wrapping
export NKHOME=$TOP_DIR/notice_vme_nfadc400

# daq_env.sh가 있으면 로드하여 컴파일러 플래그 및 라이브러리 경로 설정
ENV_SCRIPT="$DAQHOME/daq_env.sh"
if [ -f "$ENV_SCRIPT" ]; then
    echo "  -> Sourcing $ENV_SCRIPT"
    source "$ENV_SCRIPT"
else
    echo "  -> Warning: daq_env.sh not found at $ENV_SCRIPT"
    echo "     Build might fail if environment variables are missing."
fi

# ------------------------------------------------------
# 1. Notice VME/NFADC400 Driver & Libs (Kernel/Hardware)
# ------------------------------------------------------
if [ -d "$NKHOME" ]; then
    echo -e "\n>>> [Step 1] Building Notice Drivers & Libraries..."
    cd "$NKHOME"
    
    # Clean previous builds
    if [ -d "driver" ]; then cd driver; make clean >/dev/null 2>&1; cd ..; fi
    if [ -d "src/linux_interface" ]; then cd src/linux_interface; rm -f *.o *.so; cd ../..; fi
    
    # Build using build_notice_kernel.sh
    if [ -f "./build_notice_kernel.sh" ]; then
        chmod +x build_notice_kernel.sh
        ./build_notice_kernel.sh
    else
        echo "Error: 'build_notice_kernel.sh' not found in notice_vme_nfadc400!"
        exit 1
    fi
    
    if [ $? -ne 0 ]; then
        echo "Error: Notice Module build failed."
        exit 1
    fi
    cd "$TOP_DIR"
else
    echo "Error: 'notice_vme_nfadc400' directory not found!"
    exit 1
fi

# ------------------------------------------------------
# 2. DAQ Frontend & Objects (API/Application)
# ------------------------------------------------------
if [ -d "$DAQHOME" ]; then
    echo -e "\n>>> [Step 2] Building DAQ Application (API)..."
    cd "$DAQHOME"
    
    # Build using build_notice_api.sh
    if [ -f "./build_notice_api.sh" ]; then
        chmod +x build_notice_api.sh
        ./build_notice_api.sh
    else
        echo "Error: 'build_notice_api.sh' not found in vme_fadc400_wrapping!"
        exit 1
    fi
    
    if [ $? -ne 0 ]; then
        echo "Error: DAQ Application build failed."
        exit 1
    fi
    cd "$TOP_DIR"
else
    echo "Error: 'vme_fadc400_wrapping' directory not found!"
    exit 1
fi

# 빌드 완료 후 안내 메시지 출력 섹션
echo -e "\n========================================================"
echo -e "      🎉  All Installations Completed Successfully!      "
echo -e "========================================================"

echo -e "\n[!] CRITICAL CHECKLIST (Must Read):"
echo "  1. USB Connection: If you just installed the driver for the first time,"
echo "     please UNPLUG and RE-PLUG the USB cable to apply permissions."
echo "  2. Device Check: Run 'ls -l /dev/nkusbvme*' to verify device exists."
echo "  3. Environment: You MUST source the env script before running DAQ."

echo -e "\n[?] How to Configure (configs/daq.config):"
echo "  The DAQ behavior is controlled by 'vme_fadc400_wrapping/configs/daq.config'."
echo "  Key parameters to check:"
echo "    - RUNNUM : Run number (e.g., 100)"
echo "    - FADC   : [MID] [NCH] [CID...] (e.g., FADC 0 4 0 1 2 3)"
echo "    - NDP    : Number of Data Points (Sample window size)"
echo "    - TLT    : Trigger Lookup Table (0xFFFE for OR, 0x8000 for AND)"
echo "    - THR    : Threshold (ADC counts)"

echo -e "\n[>] Quick Start Commands:"
echo "  1. Setup Environment:"
echo "     source vme_fadc400_wrapping/daq_env.sh"
echo ""
echo "  2. Run DAQ (Data Acquisition):"
echo "     frontend_nfadc400 -f vme_fadc400_wrapping/configs/daq.config -o test_run"
echo ""
echo "  3. Analyze Data (Production):"
echo "     production_nfadc400 -i test_run.root -w"
echo ""
echo "========================================================"