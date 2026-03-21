#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QPushButton, 
                             QLabel, QLineEdit, QGroupBox, QSpinBox, QGridLayout, 
                             QMessageBox, QCheckBox, QScrollArea, QComboBox)
from PyQt5.QtCore import QTimer, pyqtSignal, Qt

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
        self.crate_map = []
        self.hw_timer = QTimer(self)
        self.hw_timer.timeout.connect(self.update_dashboard)
        
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

        # --- 슬롯 선택(콤보박스) 부 ---
        slot_layout = QHBoxLayout()
        self.combo_slot = QComboBox()
        self.combo_slot.setStyleSheet("font-weight: bold; padding: 5px;")
        self.combo_slot.currentIndexChanged.connect(self.on_slot_changed)
        slot_layout.addWidget(QLabel("<b>📍 Target Slot:</b>"))
        slot_layout.addWidget(self.combo_slot, stretch=1)
        caen_layout.addLayout(slot_layout)

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

    def toggle_caen(self):
        if not HAS_CAEN_LIBS: return
        if self.device is None:
            try:
                ip = self.input_ip.text().strip()
                self.device = hv.Device.open(hv.SystemType.SY4527, hv.LinkType.TCPIP, ip, "admin", "admin")
                
                # 크레이트 맵 스캔 후 콤보박스 세팅
                self.crate_map = self.device.get_crate_map()
                self.combo_slot.blockSignals(True)
                self.combo_slot.clear()
                
                for board in self.crate_map:
                    if board is not None:
                        self.combo_slot.addItem(f"Slot {board.slot} ({board.n_channel} Channels)", board.slot)
                
                self.combo_slot.blockSignals(False)

                if self.combo_slot.count() > 0:
                    self.on_slot_changed() 
                else:
                    self.sig_log.emit("[CAEN] Connected, but no boards found in crate.", True)

                self.btn_conn.setText("Disconnect")
                self.btn_conn.setStyleSheet("background-color: #F44336; color: white; font-weight:bold;")
                self.sig_log.emit(f"[CAEN] Connected to {ip} & Crate mapped.", False)
                
                # 1.5초 폴링 (현재 보이는 슬롯만 폴링)
                self.hw_timer.start(1500) 
            except Exception as e:
                QMessageBox.critical(self, "Connection Error", str(e))
        else:
            self.hw_timer.stop()
            self.device.close()
            self.device = None
            self.crate_map = []
            self.combo_slot.clear()
            self.btn_conn.setText("Connect CAEN (Auto-Scan)")
            self.btn_conn.setStyleSheet("background-color: #3F51B5; color: white; font-weight:bold;")
            self.clear_dynamic_ui()
            self.sig_log.emit("[CAEN] Disconnected.", False)

    def clear_dynamic_ui(self):
        for i in reversed(range(self.grid_channels.count())): 
            widget = self.grid_channels.itemAt(i).widget()
            if widget: widget.setParent(None)
        self.spin_v0.clear(); self.spin_i0.clear(); self.btn_pw.clear()
        self.lbl_vmon.clear(); self.lbl_imon.clear(); self.lbl_stat.clear()

    def on_slot_changed(self):
        if not self.device or self.combo_slot.currentIndex() < 0: return
        target_slot = self.combo_slot.currentData()
        
        board = next((b for b in self.crate_map if b and b.slot == target_slot), None)
        if board:
            self.build_dynamic_ui(board)

    def build_dynamic_ui(self, board):
        self.clear_dynamic_ui()
        headers = ["Channel (Name)", "V0Set (V)", "I0Set (uA)", "Apply", "Power", "VMon (V)", "IMon (uA)", "Status"]
        for col, h in enumerate(headers):
            self.grid_channels.addWidget(QLabel(f"<b>{h}</b>"), 0, col)

        ch_list = list(range(board.n_channel))
        try:
            ch_names = self.device.get_ch_name(board.slot, ch_list)
        except Exception:
            ch_names = [f"Ch{c}" for c in ch_list]

        row = 1
        for ch in ch_list:
            name = ch_names[ch] if ch_names[ch] else f"Ch{ch}"
            self.grid_channels.addWidget(QLabel(f"<b>{ch}</b> ({name})"), row, 0)
            
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
            
        self.sync_initial_settings(board)

    def sync_initial_settings(self, board):
        try:
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
        if not self.device or self.combo_slot.currentIndex() < 0: return
        target_slot = self.combo_slot.currentData()

        try:
            board = next((b for b in self.crate_map if b and b.slot == target_slot), None)
            if not board: return

            ch_list = list(range(board.n_channel))
            vmons = self.device.get_ch_param(board.slot, ch_list, "VMon")
            imons = self.device.get_ch_param(board.slot, ch_list, "IMon")
            pwrs  = self.device.get_ch_param(board.slot, ch_list, "Pw")
            
            for i, ch in enumerate(ch_list):
                if (board.slot, ch) in self.lbl_vmon: 
                    self.lbl_vmon[(board.slot, ch)].setText(f"{vmons[i]:.1f}")
                    self.lbl_imon[(board.slot, ch)].setText(f"{imons[i]:.2f}")
                    if pwrs[i]:
                        self.lbl_stat[(board.slot, ch)].setText("ON")
                        self.lbl_stat[(board.slot, ch)].setStyleSheet("color: green; font-weight: bold;")
                    else:
                        self.lbl_stat[(board.slot, ch)].setText("OFF")
                        self.lbl_stat[(board.slot, ch)].setStyleSheet("color: red; font-weight: bold;")
        except Exception:
            pass 

    def force_shutdown(self):
        # 긴급 셧다운 시에는 슬롯 상관없이 모든 보드 강제 OFF
        if self.cb_enable_scm.isChecked() and self.device:
            self.sig_log.emit("[CAEN] EMERGENCY SHUTDOWN applied.", True)
            for board in self.crate_map:
                if board is None: continue
                ch_list = list(range(board.n_channel))
                self.device.set_ch_param(board.slot, ch_list, "Pw", 0)