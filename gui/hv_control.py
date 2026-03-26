#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QPushButton, 
                             QLabel, QLineEdit, QGroupBox, QSpinBox, QGridLayout, 
                             QMessageBox, QCheckBox, QScrollArea, QComboBox)
from PyQt5.QtCore import QTimer, pyqtSignal, Qt
from PyQt5.QtGui import QFont

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

        # ==============================================================================
        # 아날로그 모드 UI
        # ==============================================================================
        self.analog_widget = QWidget()
        analog_layout = QVBoxLayout(self.analog_widget)
        
        lbl_analog_mode = QLabel("⚠️ Analog HV Mode")
        lbl_analog_mode.setFont(QFont("Arial", 18, QFont.Bold))
        lbl_analog_mode.setAlignment(Qt.AlignCenter)
        lbl_analog_mode.setStyleSheet("color: #424242; padding-top: 20px;")
        
        lbl_analog_desc = QLabel("순수 아날로그 장비(ORTEC 등) 사용 중입니다.\n소프트웨어 제어 및 통신 폴링이 중단되었습니다.")
        lbl_analog_desc.setFont(QFont("Arial", 12))
        lbl_analog_desc.setAlignment(Qt.AlignCenter)
        lbl_analog_desc.setStyleSheet("color: #616161; padding-bottom: 10px;")
        
        input_group = QGroupBox("⚡ Manual HV Input for Database")
        input_group.setFont(QFont("Arial", 12, QFont.Bold))
        input_group.setStyleSheet("""
            QGroupBox {
                border: 2px solid #90A4AE;
                border-radius: 8px;
                background-color: #FFFFFF;
                margin-top: 15px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top left;
                left: 15px;
                color: #D84315;
            }
        """)
        hv_input_layout = QHBoxLayout(input_group)
        hv_input_layout.setContentsMargins(20, 30, 20, 20)
        
        lbl_v = QLabel("Voltage (V):")
        lbl_v.setFont(QFont("Arial", 14, QFont.Bold))
        self.input_analog_hv = QLineEdit()
        self.input_analog_hv.setPlaceholderText("ex) -1700")
        self.input_analog_hv.setMinimumHeight(40)
        self.input_analog_hv.setStyleSheet("font-weight:bold; font-size: 16px; padding: 5px; border: 2px solid #CFD8DC; border-radius: 4px; background-color: #F1F8E9;")
        
        lbl_i = QLabel("Current (uA):")
        lbl_i.setFont(QFont("Arial", 14, QFont.Bold))
        self.input_analog_i = QLineEdit()
        self.input_analog_i.setPlaceholderText("ex) 10")
        self.input_analog_i.setMinimumHeight(40)
        self.input_analog_i.setStyleSheet("font-weight:bold; font-size: 16px; padding: 5px; border: 2px solid #CFD8DC; border-radius: 4px; background-color: #F1F8E9;")
        
        hv_input_layout.addStretch()
        hv_input_layout.addWidget(lbl_v)
        hv_input_layout.addWidget(self.input_analog_hv)
        hv_input_layout.addSpacing(30)
        hv_input_layout.addWidget(lbl_i)
        hv_input_layout.addWidget(self.input_analog_i)
        hv_input_layout.addStretch()

        lbl_notice = QLabel("※ 주의: 여기에 기입된 전압/전류 값은 데이터 수집(Run) 시 E-Log 데이터베이스에 자동 푸쉬(Push)되어 영구 기록됩니다.")
        lbl_notice.setAlignment(Qt.AlignCenter)
        lbl_notice.setStyleSheet("color: #D32F2F; font-weight: bold; font-size: 13px; padding-top: 15px;")
        
        analog_layout.addStretch()
        analog_layout.addWidget(lbl_analog_mode)
        analog_layout.addWidget(lbl_analog_desc)
        
        input_wrapper = QHBoxLayout()
        input_wrapper.addStretch(1)
        input_wrapper.addWidget(input_group, stretch=2)
        input_wrapper.addStretch(1)
        analog_layout.addLayout(input_wrapper)
        
        analog_layout.addWidget(lbl_notice)
        analog_layout.addStretch()
        
        self.analog_widget.setStyleSheet("background-color: #ECEFF1; border-radius: 8px;") 
        self.analog_widget.setVisible(False)
        self.main_layout.addWidget(self.analog_widget)

        # ==============================================================================
        # CAEN 제어 패널 UI
        # ==============================================================================
        self.caen_widget = QWidget()
        caen_layout = QVBoxLayout(self.caen_widget)
        caen_layout.setContentsMargins(0, 0, 0, 0)
        
        conn_layout = QHBoxLayout()
        
        # 💡 [추가] 통신 방식(TCP/IP vs USB) 선택 콤보박스
        self.combo_link = QComboBox()
        self.combo_link.addItems(["TCP/IP", "USB"])
        self.combo_link.setStyleSheet("font-weight:bold; color:#1565C0;")
        self.combo_link.currentIndexChanged.connect(self.on_link_type_changed)
        
        self.input_ip = QLineEdit("192.168.0.10")
        self.input_user = QLineEdit("admin")
        self.input_pass = QLineEdit("admin")
        self.input_pass.setEchoMode(QLineEdit.Password) 
        self.input_user.setMaximumWidth(80)
        self.input_pass.setMaximumWidth(80)
        
        self.btn_conn = QPushButton("Connect CAEN (Auto-Scan)")
        self.btn_conn.setStyleSheet("background-color: #3F51B5; color: white; font-weight:bold;")
        self.btn_conn.clicked.connect(self.toggle_caen)
        
        conn_layout.addWidget(QLabel("Link:"))
        conn_layout.addWidget(self.combo_link)
        self.lbl_ip_node = QLabel("IP:")
        conn_layout.addWidget(self.lbl_ip_node)
        conn_layout.addWidget(self.input_ip, stretch=1)
        conn_layout.addWidget(QLabel("User:"))
        conn_layout.addWidget(self.input_user)
        conn_layout.addWidget(QLabel("Pass:"))
        conn_layout.addWidget(self.input_pass)
        conn_layout.addWidget(self.btn_conn)
        caen_layout.addLayout(conn_layout)

        # 💡 [수정] 박스 여백(Padding)을 획기적으로 줄여 시인성을 확보했습니다.
        self.caen_fallback_widget = QGroupBox("⚠️ Manual Input for DB (Server Offline / Exclusive Mode)")
        self.caen_fallback_widget.setFont(QFont("Arial", 10, QFont.Bold))
        self.caen_fallback_widget.setStyleSheet("""
            QGroupBox { 
                border: 2px solid #FFB300; 
                border-radius: 6px; 
                background-color: #FFF8E1; 
                margin-top: 10px; 
                margin-bottom: 5px;
            }
            QGroupBox::title { 
                subcontrol-origin: margin; 
                subcontrol-position: top left; 
                left: 10px; 
                color: #E65100; 
            }
        """)
        fb_layout = QHBoxLayout(self.caen_fallback_widget)
        fb_layout.setContentsMargins(10, 15, 10, 8) # 위아래 여백을 꽉 조였습니다!
        
        lbl_fb_notice = QLabel("※ 연결 실패 시, 아래 기입된 값이 런(Run) 기록 시 DB로 푸쉬됩니다.")
        lbl_fb_notice.setStyleSheet("color: #D32F2F; font-weight: bold;")
        fb_layout.addWidget(lbl_fb_notice)
        fb_layout.addStretch()
        
        self.input_caen_fallback_v = QLineEdit()
        self.input_caen_fallback_v.setPlaceholderText("Voltage (V)")
        self.input_caen_fallback_v.setMinimumHeight(28)
        self.input_caen_fallback_v.setStyleSheet("font-weight:bold; font-size: 13px; padding: 2px; border: 1px solid #B0BEC5; background-color: white;")
        fb_layout.addWidget(self.input_caen_fallback_v)
        
        self.input_caen_fallback_i = QLineEdit()
        self.input_caen_fallback_i.setPlaceholderText("Current (uA)")
        self.input_caen_fallback_i.setMinimumHeight(28)
        self.input_caen_fallback_i.setStyleSheet("font-weight:bold; font-size: 13px; padding: 2px; border: 1px solid #B0BEC5; background-color: white;")
        fb_layout.addWidget(self.input_caen_fallback_i)
        
        caen_layout.addWidget(self.caen_fallback_widget)

        if not HAS_CAEN_LIBS:
            warn = QLabel("caen-libs 패키지가 설치되지 않았습니다. (pip install caen-libs 필요)")
            warn.setStyleSheet("color: red; font-weight: bold;")
            caen_layout.addWidget(warn)

        slot_layout = QHBoxLayout()
        self.combo_slot = QComboBox()
        self.combo_slot.setStyleSheet("font-weight: bold; padding: 5px;")
        self.combo_slot.currentIndexChanged.connect(self.on_slot_changed)
        slot_layout.addWidget(QLabel("<b>📍 Target Slot:</b>"))
        slot_layout.addWidget(self.combo_slot, stretch=1)
        caen_layout.addLayout(slot_layout)

        self.scroll_area = QScrollArea()
        self.scroll_area.setWidgetResizable(True)
        self.scroll_content = QWidget()
        self.grid_channels = QGridLayout(self.scroll_content)
        self.scroll_area.setWidget(self.scroll_content)
        
        caen_layout.addWidget(self.scroll_area)
        self.main_layout.addWidget(self.caen_widget)

    # 💡 [추가] 통신 방식이 바뀔 때 불필요한 입력을 잠그는 스마트 UI 로직
    def on_link_type_changed(self):
        if self.combo_link.currentText() == "USB":
            self.lbl_ip_node.setText("Node:")
            self.input_ip.setText("0") # 보통 USB는 Node 0번 사용
            self.input_user.setEnabled(False)
            self.input_pass.setEnabled(False)
            self.input_user.setStyleSheet("background-color: #E0E0E0; color: #9E9E9E;")
            self.input_pass.setStyleSheet("background-color: #E0E0E0; color: #9E9E9E;")
        else:
            self.lbl_ip_node.setText("IP:")
            self.input_ip.setText("192.168.0.10")
            self.input_user.setEnabled(True)
            self.input_pass.setEnabled(True)
            self.input_user.setStyleSheet("")
            self.input_pass.setStyleSheet("")

    def get_current_hv(self):
        if self.cb_enable_scm.isChecked():
            if self.device:
                try:
                    for board in self.crate_map:
                        if board:
                            vmon = self.device.get_ch_param(board.slot, [0], "VMon")[0]
                            return f"{vmon:.1f}V (CAEN)"
                except Exception:
                    pass
                return "CAEN (Online)"
            else:
                v = self.input_caen_fallback_v.text().strip()
                i = self.input_caen_fallback_i.text().strip()
                if v and i: return f"{v}V, {i}uA (Manual)"
                elif v: return f"{v}V (Manual)"
                else: return "N/A"
        else:
            v = self.input_analog_hv.text().strip()
            i = self.input_analog_i.text().strip()
            if v and i: return f"{v}V, {i}uA"
            elif v: return f"{v}V"
            else: return "N/A"

    def toggle_scm_mode(self, checked):
        self.caen_widget.setVisible(checked)
        self.analog_widget.setVisible(not checked)
        if not checked and self.device is not None:
            self.toggle_caen() 

    def toggle_caen(self):
        if not HAS_CAEN_LIBS: return
        if self.device is None:
            try:
                ip_or_node = self.input_ip.text().strip()
                user = self.input_user.text().strip()
                pwd = self.input_pass.text().strip()
                
                # 💡 선택된 통신 방식(TCPIP vs USB)에 따라 분기
                link_type = hv.LinkType.TCPIP if self.combo_link.currentText() == "TCP/IP" else hv.LinkType.USB
                
                self.device = hv.Device.open(hv.SystemType.SY4527, link_type, ip_or_node, user, pwd)
                
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
                self.sig_log.emit(f"[CAEN] Connected to {ip_or_node} via {self.combo_link.currentText()}.", False)
                self.caen_fallback_widget.setVisible(False) 
                self.hw_timer.start(1500) 
            except Exception as e:
                self.caen_fallback_widget.setVisible(True) 
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
            self.caen_fallback_widget.setVisible(True) 
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
        if self.cb_enable_scm.isChecked() and self.device:
            self.sig_log.emit("[CAEN] EMERGENCY SHUTDOWN applied.", True)
            for board in self.crate_map:
                if board is None: continue
                ch_list = list(range(board.n_channel))
                self.device.set_ch_param(board.slot, ch_list, "Pw", 0)