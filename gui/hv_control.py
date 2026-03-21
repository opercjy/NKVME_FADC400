#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QPushButton, 
                             QLabel, QLineEdit, QGroupBox, QSpinBox, QGridLayout, 
                             QMessageBox, QCheckBox, QScrollArea)
from PyQt5.QtCore import QTimer, pyqtSignal, Qt

# CAEN 공식 라이브러리 임포트 시도
try:
    from caen_libs import caenhvwrapper as hv
    HAS_CAEN_LIBS = True
except ImportError:
    HAS_CAEN_LIBS = False

class HVControlPanel(QWidget):
    sig_log = pyqtSignal(str, bool)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.device = None
        self.hw_timer = QTimer(self)
        self.hw_timer.timeout.connect(self.update_dashboard)
        
        # 동적 UI 위젯들을 저장할 딕셔너리 {(slot, ch): widget}
        self.spin_v0 = {}
        self.spin_i0 = {}
        self.btn_pw = {}
        self.lbl_vmon = {}
        self.lbl_imon = {}
        self.lbl_stat = {}

        self.init_ui()

    def init_ui(self):
        self.main_layout = QVBoxLayout(self)

        self.cb_enable_scm = QCheckBox("🔌 CAEN HV Slow Control 기능 활성화 (아날로그 장비 사용 시 체크 해제)")
        self.cb_enable_scm.setStyleSheet("font-weight: bold; color: #3F51B5; padding: 5px;")
        self.cb_enable_scm.setChecked(True)
        self.cb_enable_scm.toggled.connect(self.toggle_scm_mode)
        self.main_layout.addWidget(self.cb_enable_scm)

        self.lbl_analog_mode = QLabel("⚠️ Analog HV Mode\n\n순수 아날로그 장비(ORTEC 등) 사용 중입니다.\n소프트웨어 제어 및 폴링이 중단되었습니다.")
        self.lbl_analog_mode.setAlignment(Qt.AlignCenter)
        self.lbl_analog_mode.setStyleSheet("font-weight: bold; color: #795548; background-color: #FFF3E0; padding: 20px;")
        self.lbl_analog_mode.setVisible(False)
        self.main_layout.addWidget(self.lbl_analog_mode)

        self.caen_widget = QWidget()
        caen_layout = QVBoxLayout(self.caen_widget)
        caen_layout.setContentsMargins(0, 0, 0, 0)
        
        # --- 상단 연결부 ---
        conn_layout = QHBoxLayout()
        self.input_ip = QLineEdit("192.168.0.10")
        self.btn_conn = QPushButton("Connect CAEN (Auto-Scan)")
        self.btn_conn.setStyleSheet("background-color: #3F51B5; color: white; font-weight:bold;")
        self.btn_conn.clicked.connect(self.toggle_caen)
        conn_layout.addWidget(QLabel("Mainframe IP:"))
        conn_layout.addWidget(self.input_ip)
        conn_layout.addWidget(self.btn_conn)
        caen_layout.addLayout(conn_layout)

        if not HAS_CAEN_LIBS:
            warn = QLabel("caen-libs 패키지가 설치되지 않았습니다. (pip install caen-libs 필요)")
            warn.setStyleSheet("color: red; font-weight: bold;")
            caen_layout.addWidget(warn)

        # --- 동적 채널 패널 (스크롤 영역) ---
        self.scroll_area = QScrollArea()
        self.scroll_area.setWidgetResizable(True)
        self.scroll_content = QWidget()
        self.grid_channels = QGridLayout(self.scroll_content)
        self.scroll_area.setWidget(self.scroll_content)
        
        caen_layout.addWidget(self.scroll_area)
        self.main_layout.addWidget(self.caen_widget)

    def toggle_scm_mode(self, checked):
        self.caen_widget.setVisible(checked)
        self.lbl_analog_mode.setVisible(not checked)
        if not checked and self.device is not None:
            self.toggle_caen() 

    def clear_dynamic_ui(self):
        for i in reversed(range(self.grid_channels.count())): 
            self.grid_channels.itemAt(i).widget().setParent(None)
        self.spin_v0.clear(); self.spin_i0.clear(); self.btn_pw.clear()
        self.lbl_vmon.clear(); self.lbl_imon.clear(); self.lbl_stat.clear()

    def build_dynamic_ui(self):
        """연결된 보드와 채널을 읽어와서 동적으로 UI를 구성합니다."""
        self.clear_dynamic_ui()
        headers = ["Slot.Ch (Name)", "V0Set (V)", "I0Set (uA)", "Apply", "Power", "VMon (V)", "IMon (uA)", "Status"]
        for col, h in enumerate(headers):
            self.grid_channels.addWidget(QLabel(f"<b>{h}</b>"), 0, col)

        slots = self.device.get_crate_map()
        row = 1
        for board in slots:
            if board is None: continue
            
            # 채널 이름 한 번에 가져오기
            ch_list = list(range(board.n_channel))
            ch_names = self.device.get_ch_name(board.slot, ch_list)

            for ch in ch_list:
                name = ch_names[ch] if ch_names[ch] else f"Ch{ch}"
                self.grid_channels.addWidget(QLabel(f"<b>S{board.slot}.{ch}</b> ({name})"), row, 0)
                
                sp_v = QSpinBox(); sp_v.setRange(0, 4000)
                self.spin_v0[(board.slot, ch)] = sp_v
                self.grid_channels.addWidget(sp_v, row, 1)

                sp_i = QSpinBox(); sp_i.setRange(0, 5000)
                self.spin_i0[(board.slot, ch)] = sp_i
                self.grid_channels.addWidget(sp_i, row, 2)

                btn_set = QPushButton("Set")
                btn_set.clicked.connect(lambda _, s=board.slot, c=ch: self.set_channel_params(s, c))
                self.grid_channels.addWidget(btn_set, row, 3)

                btn_pw = QPushButton("ON")
                btn_pw.setStyleSheet("background-color: #4CAF50; color: white;")
                btn_pw.clicked.connect(lambda _, s=board.slot, c=ch: self.toggle_power(s, c))
                self.btn_pw[(board.slot, ch)] = btn_pw
                self.grid_channels.addWidget(btn_pw, row, 4)

                lbl_v = QLabel("0.0"); lbl_v.setStyleSheet("font-family: monospace; font-weight: bold;")
                self.lbl_vmon[(board.slot, ch)] = lbl_v
                self.grid_channels.addWidget(lbl_v, row, 5)

                lbl_i = QLabel("0.00"); lbl_i.setStyleSheet("font-family: monospace;")
                self.lbl_imon[(board.slot, ch)] = lbl_i
                self.grid_channels.addWidget(lbl_i, row, 6)

                lbl_s = QLabel("OFF"); lbl_s.setStyleSheet("color:red; font-weight:bold;")
                self.lbl_stat[(board.slot, ch)] = lbl_s
                self.grid_channels.addWidget(lbl_s, row, 7)
                
                row += 1
                
        # 초기 설정값 불러오기
        self.sync_initial_settings(slots)

    def sync_initial_settings(self, slots):
        try:
            for board in slots:
                if board is None: continue
                ch_list = list(range(board.n_channel))
                vsets = self.device.get_ch_param(board.slot, ch_list, "V0Set")
                isets = self.device.get_ch_param(board.slot, ch_list, "I0Set")
                pwrs  = self.device.get_ch_param(board.slot, ch_list, "Pw")
                
                for i, ch in enumerate(ch_list):
                    self.spin_v0[(board.slot, ch)].setValue(int(vsets[i]))
                    self.spin_i0[(board.slot, ch)].setValue(int(isets[i]))
                    if pwrs[i]:
                        self.btn_pw[(board.slot, ch)].setText("OFF")
                        self.btn_pw[(board.slot, ch)].setStyleSheet("background-color: #F44336; color: white;")
        except Exception as e:
            self.sig_log.emit(f"[HV] Sync Error: {e}", True)

    def toggle_caen(self):
        if not HAS_CAEN_LIBS: return
        if self.device is None:
            try:
                ip = self.input_ip.text().strip()
                self.device = hv.Device.open(hv.SystemType.SY4527, hv.LinkType.TCPIP, ip, "admin", "admin")
                self.build_dynamic_ui()
                self.btn_conn.setText("Disconnect")
                self.btn_conn.setStyleSheet("background-color: #F44336; color: white; font-weight:bold;")
                self.sig_log.emit(f"[CAEN] Connected to {ip} & Crate mapped.", False)
                self.hw_timer.start(1500) # 1.5초 폴링 (부하 방지)
            except Exception as e:
                QMessageBox.critical(self, "Connection Error", str(e))
        else:
            self.hw_timer.stop()
            self.device.close()
            self.device = None
            self.btn_conn.setText("Connect CAEN (Auto-Scan)")
            self.btn_conn.setStyleSheet("background-color: #3F51B5; color: white; font-weight:bold;")
            self.clear_dynamic_ui()
            self.sig_log.emit("[CAEN] Disconnected.", False)

    def set_channel_params(self, slot, ch):
        if not self.device: return
        v0 = self.spin_v0[(slot, ch)].value()
        i0 = self.spin_i0[(slot, ch)].value()
        try:
            self.device.set_ch_param(slot, [ch], "V0Set", float(v0))
            self.device.set_ch_param(slot, [ch], "I0Set", float(i0))
            self.sig_log.emit(f"[HV] S{slot}.{ch} -> V0Set: {v0}V, I0Set: {i0}uA", False)
        except Exception as e:
            self.sig_log.emit(f"[HV] Param Set Error: {e}", True)

    def toggle_power(self, slot, ch):
        if not self.device: return
        btn = self.btn_pw[(slot, ch)]
        turn_on = (btn.text() == "ON")
        try:
            self.device.set_ch_param(slot, [ch], "Pw", 1 if turn_on else 0)
            if turn_on:
                btn.setText("OFF"); btn.setStyleSheet("background-color: #F44336; color: white;")
                self.sig_log.emit(f"[HV] S{slot}.{ch} Power ON.", False)
            else:
                btn.setText("ON"); btn.setStyleSheet("background-color: #4CAF50; color: white;")
                self.sig_log.emit(f"[HV] S{slot}.{ch} Power OFF.", False)
        except Exception as e:
            self.sig_log.emit(f"[HV] Power Toggle Error: {e}", True)

    def update_dashboard(self):
        if not self.device: return
        try:
            slots = self.device.get_crate_map()
            for board in slots:
                if board is None: continue
                ch_list = list(range(board.n_channel))
                vmons = self.device.get_ch_param(board.slot, ch_list, "VMon")
                imons = self.device.get_ch_param(board.slot, ch_list, "IMon")
                pwrs  = self.device.get_ch_param(board.slot, ch_list, "Pw")
                
                for i, ch in enumerate(ch_list):
                    self.lbl_vmon[(board.slot, ch)].setText(f"{vmons[i]:.1f}")
                    self.lbl_imon[(board.slot, ch)].setText(f"{imons[i]:.2f}")
                    if pwrs[i]:
                        self.lbl_stat[(board.slot, ch)].setText("ON")
                        self.lbl_stat[(board.slot, ch)].setStyleSheet("color: green; font-weight: bold;")
                    else:
                        self.lbl_stat[(board.slot, ch)].setText("OFF")
                        self.lbl_stat[(board.slot, ch)].setStyleSheet("color: red; font-weight: bold;")
        except Exception:
            pass # 일시적인 통신 끊김 무시

    def force_shutdown(self):
        if self.cb_enable_scm.isChecked() and self.device:
            self.sig_log.emit("[CAEN] EMERGENCY SHUTDOWN applied.", True)
            slots = self.device.get_crate_map()
            for board in slots:
                if board is None: continue
                ch_list = list(range(board.n_channel))
                self.device.set_ch_param(board.slot, ch_list, "Pw", 0) # 0 전달로 모두 강제 OFF