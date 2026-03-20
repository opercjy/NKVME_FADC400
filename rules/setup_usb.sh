#!/bin/bash
# USB-VME 통신 권한 자동 등록 스크립트

RULE_FILE="99-nkusbvme.rules"
DEST_DIR="/etc/udev/rules.d"

echo "[INFO] Notice USB-VME UDEV 룰을 시스템에 등록합니다."
echo "[INFO] 관리자(root) 권한이 필요합니다."

sudo cp ${RULE_FILE} ${DEST_DIR}/
sudo udevadm control --reload-rules
sudo udevadm trigger

echo "[SUCCESS] USB-VME 권한 설정이 완료되었습니다. USB 케이블을 뺐다가 다시 연결해 주세요."
