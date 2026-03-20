#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QPushButton, 
                             QLabel, QLineEdit, QGroupBox, QSpinBox, QGridLayout, 
                             QMessageBox, QCheckBox)
from PyQt5.QtCore import QTimer, pyqtSignal, Qt

# ==========================================================
# 1. CAEN HV 하드웨어 래퍼 (Hardware Abstraction Layer)
# ==========================================================
class CaenHVController:
    def __init__(self):
        self.is_connected = False
        self._dummy_vmon = [0.0] * 4; self._dummy_imon = [0.0] * 4; self._dummy_pwon = [False] * 4

    def connect(self, ip):
        # TODO: 실제 CAEN 라이브러리 연결 로직
        self.is_connected = True
        return True

    def disconnect(self):
        self.is_connected = False

    def power_on(self, ch): self._dummy_pwon[ch] = True
    def power_off(self, ch): self._dummy_pwon[ch] = False
    def set_voltage(self, ch, v0): self._dummy_vmon[ch] = float(v0)

    def get_status(self):
        if not self.is_connected: return None
        for i in range(4):
            if self._dummy_pwon[i]: self._dummy_imon[i] = 1.23
            else: self._dummy_vmon[i] *= 0.8; self._dummy_imon[i] = 0.0
        return self._dummy_vmon, self._dummy_imon, self._dummy_pwon

# ==========================================================
# 2. 통합 하드웨어 컨트롤 패널 위젯 (마스터 토글 적용)
# ==========================================================
class HVControlPanel(QWidget):
    sig_log = pyqtSignal(str, bool)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.caen = CaenHVController()
        self.hw_timer = QTimer(self)
        self.hw_timer.timeout.connect(self.update_dashboard)
        self.init_ui()

    def init_ui(self):
        self.main_layout = QVBoxLayout(self)

        # --- [1] HV SCM 마스터 스위치 ---
        self.cb_enable_scm = QCheckBox("🔌 CAEN HV Slow Control 기능 활성화 (아날로그 장비 사용 시 체크 해제)")
        self.cb_enable_scm.setStyleSheet("font-size: 14px; font-weight: bold; color: #3F51B5; padding: 10px;")
        self.cb_enable_scm.setChecked(True)
        self.cb_enable_scm.toggled.connect(self.toggle_scm_mode)
        self.main_layout.addWidget(self.cb_enable_scm)

        # --- [2] 아날로그 수동 모드 안내 메시지 (평소엔 숨김) ---
        self.lbl_analog_mode = QLabel("⚠️ Analog HV Mode\n\n현재 순수 아날로그 장비(ORTEC 등)를 사용 중입니다.\n물리적 다이얼을 통해 수동으로 전압을 조절해 주십시오.\n(소프트웨어 폴링 및 통신 중단됨)")
        self.lbl_analog_mode.setAlignment(Qt.AlignCenter)
        self.lbl_analog_mode.setStyleSheet("font-size: 16px; font-weight: bold; color: #795548; background-color: #FFF3E0; padding: 30px; border-radius: 10px;")
        self.lbl_analog_mode.setVisible(False)
        self.main_layout.addWidget(self.lbl_analog_mode)

        # --- [3] CAEN 패널들을 담을 레이아웃 ---
        self.caen_widget = QWidget()
        caen_main_layout = QHBoxLayout(self.caen_widget)
        caen_main_layout.setContentsMargins(0, 0, 0, 0)
        
        # 3-1. 제어부
        self.caen_group = QGroupBox("CAEN SY4527 Mainframe Control")
        caen_layout = QVBoxLayout()
        
        conn_layout = QHBoxLayout()
        self.input_caen_ip = QLineEdit("192.168.0.10")
        self.btn_caen_conn = QPushButton("Connect CAEN")
        self.btn_caen_conn.setStyleSheet("background-color: #3F51B5; color: white;")
        self.btn_caen_conn.clicked.connect(self.toggle_caen)
        conn_layout.addWidget(QLabel("IP:")); conn_layout.addWidget(self.input_caen_ip); conn_layout.addWidget(self.btn_caen_conn)
        caen_layout.addLayout(conn_layout)

        grid_caen = QGridLayout()
        self.caen_spins = []; self.btn_caen_pws = []; self.lbl_caen_v = []; self.lbl_caen_i = []; self.lbl_caen_s = []
        grid_caen.addWidget(QLabel("<b>CH</b>"), 0, 0); grid_caen.addWidget(QLabel("<b>Set V0</b>"), 0, 1)
        grid_caen.addWidget(QLabel("<b>Apply</b>"), 0, 2); grid_caen.addWidget(QLabel("<b>Power</b>"), 0, 3)

        for i in range(4):
            grid_caen.addWidget(QLabel(f"Ch {i}"), i+1, 0)
            spin = QSpinBox(); spin.setRange(0, 3000); self.caen_spins.append(spin); grid_caen.addWidget(spin, i+1, 1)
            
            btn_set = QPushButton("Set")
            btn_set.clicked.connect(lambda _, ch=i: self.set_voltage(ch))
            grid_caen.addWidget(btn_set, i+1, 2)

            btn_pw = QPushButton("ON")
            btn_pw.setStyleSheet("background-color: #4CAF50; color: white;")
            btn_pw.clicked.connect(lambda _, ch=i: self.toggle_caen_power(ch))
            self.btn_caen_pws.append(btn_pw); grid_caen.addWidget(btn_pw, i+1, 3)

        caen_layout.addLayout(grid_caen)
        caen_layout.addStretch()
        self.caen_group.setLayout(caen_layout)
        caen_main_layout.addWidget(self.caen_group)

        # 3-2. 모니터링부
        self.dash_group = QGroupBox("CAEN Live Status")
        self.dash_group.setStyleSheet("QGroupBox { border: 2px solid #FF9800; }")
        dash_layout = QVBoxLayout()
        
        grid_dash = QGridLayout()
        grid_dash.addWidget(QLabel("<b>CH</b>"), 0, 0); grid_dash.addWidget(QLabel("<b>VMon (V)</b>"), 0, 1)
        grid_dash.addWidget(QLabel("<b>IMon (uA)</b>"), 0, 2); grid_dash.addWidget(QLabel("<b>Status</b>"), 0, 3)

        for i in range(4):
            grid_dash.addWidget(QLabel(f"Ch {i}"), i+1, 0)
            lbl_v = QLabel("0.0"); lbl_v.setStyleSheet("font-family: monospace; font-weight: bold;"); self.lbl_caen_v.append(lbl_v); grid_dash.addWidget(lbl_v, i+1, 1)
            lbl_i = QLabel("0.00"); lbl_i.setStyleSheet("font-family: monospace;"); self.lbl_caen_i.append(lbl_i); grid_dash.addWidget(lbl_i, i+1, 2)
            lbl_s = QLabel("OFF"); lbl_s.setStyleSheet("color:red; font-weight:bold;"); self.lbl_caen_s.append(lbl_s); grid_dash.addWidget(lbl_s, i+1, 3)

        dash_layout.addLayout(grid_dash)
        dash_layout.addStretch()
        self.dash_group.setLayout(dash_layout)
        caen_main_layout.addWidget(self.dash_group)

        self.main_layout.addWidget(self.caen_widget)
        self.main_layout.addStretch()

    # --- [핵심] SCM 활성/비활성 예외 처리 ---
    def toggle_scm_mode(self, checked):
        """ 아날로그 장비 사용 시 체크 해제하면 CAEN 기능과 타이머를 완전히 비활성화 """
        self.caen_widget.setVisible(checked)
        self.lbl_analog_mode.setVisible(not checked)
        
        if not checked:
            # 안전을 위해 연결 해제 및 폴링 타이머 종료
            if self.caen.is_connected:
                self.toggle_caen() 
            self.sig_log.emit("[HV] Switched to Analog Manual Mode. HV communication disabled.", False)
        else:
            self.sig_log.emit("[HV] CAEN HV Slow Control Mode Enabled.", False)

    # --- 제어 로직 ---
    def toggle_caen(self):
        if not self.caen.is_connected:
            if self.caen.connect(self.input_caen_ip.text()):
                self.btn_caen_conn.setText("Disconnect")
                self.btn_caen_conn.setStyleSheet("background-color: #F44336; color: white;")
                self.sig_log.emit("[CAEN] Connected.", False)
                if not self.hw_timer.isActive(): self.hw_timer.start(1000)
        else:
            self.caen.disconnect()
            self.btn_caen_conn.setText("Connect CAEN")
            self.btn_caen_conn.setStyleSheet("background-color: #3F51B5; color: white;")
            self.hw_timer.stop()
            self.sig_log.emit("[CAEN] Disconnected.", False)

    def set_voltage(self, ch):
        if not self.caen.is_connected: return
        v0 = self.caen_spins[ch].value()
        self.caen.set_voltage(ch, v0)
        self.sig_log.emit(f"[CAEN] Ch {ch} Set V0: {v0}V", False)

    def toggle_caen_power(self, ch):
        if not self.caen.is_connected: return
        btn = self.btn_caen_pws[ch]
        if btn.text() == "ON":
            self.caen.set_voltage(ch, self.caen_spins[ch].value())
            self.caen.power_on(ch)
            btn.setText("OFF"); btn.setStyleSheet("background-color: #F44336; color: white;")
            self.sig_log.emit(f"[CAEN] Ch {ch} Power ON.", False)
        else:
            self.caen.power_off(ch)
            btn.setText("ON"); btn.setStyleSheet("background-color: #4CAF50; color: white;")
            self.sig_log.emit(f"[CAEN] Ch {ch} Power OFF.", False)

    def update_dashboard(self):
        res = self.caen.get_status()
        if res:
            cv, ci, cp = res
            for i in range(4):
                self.lbl_caen_v[i].setText(f"{cv[i]:.1f}")
                self.lbl_caen_i[i].setText(f"{ci[i]:.2f}")
                self.lbl_caen_s[i].setText("ON" if cp[i] else "OFF")
                self.lbl_caen_s[i].setStyleSheet("color: green;" if cp[i] else "color: red;")

    def force_shutdown(self):
        if self.cb_enable_scm.isChecked() and self.caen.is_connected:
            self.sig_log.emit("[CAEN] EMERGENCY SHUTDOWN applied.", True)
            for i in range(4): self.caen.power_off(i)
