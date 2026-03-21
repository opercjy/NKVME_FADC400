#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import sqlite3
import glob
import re
import shutil
from datetime import datetime
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                             QHBoxLayout, QPushButton, QLabel, QLineEdit, 
                             QGroupBox, QMessageBox, QTextEdit, QComboBox, 
                             QLCDNumber, QProgressBar, QTabWidget, QSpinBox, QRadioButton, QButtonGroup)
from PyQt5.QtCore import QProcess, Qt, QRegExp, QTimer
from PyQt5.QtGui import QRegExpValidator, QTextCursor, QFont

from hv_control import HVControlPanel
from tlu_simulator import TLUSimulatorWidget
from config_manager import ConfigManagerWidget
from prod_manager import ProductionManagerWidget
from elog_manager import ELogbookWidget 

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(CURRENT_DIR) 
DB_NAME = os.path.join(PROJECT_ROOT, "daq_history.db")
CONFIG_DIR = os.path.join(PROJECT_ROOT, "config")
TEMP_CONFIG = os.path.join(CONFIG_DIR, "temp_auto.config")

EXE_FRONTEND = "frontend_nfadc400"
EXE_MONITOR = "OnlineMonitor_nfadc400"

class DAQControlCenter(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("6UVME+NFADC400 Central Control")
        self.setMinimumSize(1250, 900)
        self.data_output_dir = PROJECT_ROOT
        
        self.daq_start_timestamp = None
        self.final_events = "0"
        self.final_time = "0.0"
        self.final_rate = "0.0"
        
        self.daq_process = QProcess(self)
        self.daq_process.readyReadStandardOutput.connect(self.handle_stdout)
        self.daq_process.readyReadStandardError.connect(self.handle_stderr)
        self.daq_process.finished.connect(self.process_finished)
        
        self.monitor_process = QProcess(self)
        
        # [핵심] 이제 모니터의 로그도 대시보드 콘솔로 가로채옵니다!
        self.monitor_process.readyReadStandardOutput.connect(self.handle_mon_stdout)
        self.monitor_process.readyReadStandardError.connect(self.handle_mon_stderr)

        self.auto_mode = "NONE" 
        self.scan_queue = []; self.scan_current_val = 0
        
        self.scan_timer = QTimer(self)
        self.scan_timer.timeout.connect(self.trigger_scan_timeout)
        self.subrun_timer = QTimer(self)
        self.subrun_timer.timeout.connect(self.trigger_subrun_rotation)
        self.manual_timer = QTimer(self)
        self.manual_timer.timeout.connect(self.trigger_manual_timeout)
        
        self.current_base_runnum = ""; self.current_subrun_idx = 1; self.subrun_max_idx = 1

        self.init_ui()

        self.clock_timer = QTimer(self)
        self.clock_timer.timeout.connect(self.update_clock)
        self.clock_timer.start(1000)

    def init_ui(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)

        top_layout = QHBoxLayout()
        title_lbl = QLabel("6UVME+NFADC400 Central Control")
        title_lbl.setFont(QFont("Arial", 18, QFont.Bold))
        self.clock_lbl = QLabel("0000-00-00 00:00:00")
        self.clock_lbl.setFont(QFont("Consolas", 14, QFont.Bold))
        top_layout.addWidget(title_lbl); top_layout.addStretch(); top_layout.addWidget(self.clock_lbl)
        main_layout.addLayout(top_layout)

        middle_layout = QHBoxLayout()
        left_panel = QVBoxLayout()
        left_panel.setContentsMargins(0, 0, 5, 0)
        
        self.tabs = QTabWidget()
        self.tabs.setStyleSheet("""
            QTabWidget::pane { border: 1px solid #CCCCCC; top: -1px; }
            QTabBar::tab { font-size: 13px; font-weight: bold; padding: 6px 15px; margin-right: 2px; background: #E0E0E0; border: 1px solid #CCCCCC; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; }
            QTabBar::tab:selected { background-color: #3F51B5; color: white; border-color: #3F51B5; }
        """)
        
        self.daq_tabs = QTabWidget()
        self.daq_tabs.setStyleSheet("""
            QTabWidget::pane { border: none; }
            QTabBar::tab { font-size: 12px; font-weight: bold; padding: 4px 12px; background-color: #E3F2FD; color: #1565C0; margin-top: 2px; margin-right: 2px; border: 1px solid #90CAF9; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; }
            QTabBar::tab:selected { background-color: #1976D2; color: white; border-color: #1976D2; }
        """)
        
        self.init_daq_tabs() 
        self.tabs.addTab(self.daq_tabs, "🚀 DAQ Controls")
        
        self.hv_panel = HVControlPanel()
        self.hv_panel.sig_log.connect(self.print_log)
        self.tabs.addTab(self.hv_panel, "⚡ HV Control")

        self.tlu_panel = TLUSimulatorWidget()
        self.tabs.addTab(self.tlu_panel, "🎯 TLU Simulator")

        self.config_panel = ConfigManagerWidget(CONFIG_DIR, self.data_output_dir)
        self.config_panel.sig_config_updated.connect(self.refresh_configs)
        self.config_panel.sig_dir_changed.connect(self.update_data_dir)
        self.tabs.addTab(self.config_panel, "📁 Config & Path")

        self.prod_panel = ProductionManagerWidget(CURRENT_DIR, self.data_output_dir)
        self.prod_panel.sig_log.connect(self.print_log)
        self.tabs.addTab(self.prod_panel, "📦 Offline Production")

        self.elog_panel = ELogbookWidget(DB_NAME)
        self.elog_panel.sig_log.connect(self.print_log)
        self.tabs.addTab(self.elog_panel, "🗄️ Database & E-Log")

        self.tabs.currentChanged.connect(lambda _: self.adjustSize())
        left_panel.addWidget(self.tabs, stretch=1)

        btn_group = QGroupBox("System Master Controls")
        btn_layout = QHBoxLayout()
        btn_layout.setContentsMargins(5, 5, 5, 5)
        
        self.btn_start_mon = QPushButton("📊 Start Monitor")
        self.btn_start_mon.setStyleSheet("background-color: #2196F3; color: white; font-weight: bold; padding: 8px;")
        self.btn_start_mon.clicked.connect(self.start_monitor)
        
        self.btn_stop_mon = QPushButton("🛑 Stop Monitor")
        self.btn_stop_mon.setStyleSheet("background-color: #9E9E9E; color: white; font-weight: bold; padding: 8px;")
        self.btn_stop_mon.clicked.connect(self.stop_monitor)
        
        self.btn_stop = QPushButton("🛑 ABORT / STOP")
        self.btn_stop.setStyleSheet("background-color: #E53935; color: white; font-weight: bold; padding: 8px;")
        self.btn_stop.clicked.connect(self.force_stop_daq)
        
        btn_layout.addWidget(self.btn_start_mon); btn_layout.addWidget(self.btn_stop_mon); btn_layout.addWidget(self.btn_stop)
        btn_group.setLayout(btn_layout); left_panel.addWidget(btn_group); middle_layout.addLayout(left_panel, stretch=6)

        right_panel = QVBoxLayout()
        right_panel.setContentsMargins(5, 0, 0, 0)
        
        dash_group = QGroupBox("Live Status Dashboard")
        dash_group.setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #9E9E9E; }")
        dash_layout = QVBoxLayout()
        dash_layout.setContentsMargins(15, 15, 15, 15) 
        dash_layout.setSpacing(10) 
        
        self.lbl_dash_mode = QLabel("IDLE")
        self.lbl_dash_mode.setFont(QFont("Arial", 14, QFont.Bold)); self.lbl_dash_mode.setStyleSheet("color: #FF9800;")
        mode_layout = QHBoxLayout(); mode_layout.addWidget(QLabel("Current Mode:")); mode_layout.addWidget(self.lbl_dash_mode)
        dash_layout.addLayout(mode_layout)
        
        time_layout = QHBoxLayout()
        self.lbl_start_time = QLabel("Start: --:--:--")
        self.lbl_elapsed_time = QLabel("Elapsed: 00:00:00")
        self.lbl_start_time.setStyleSheet("font-weight: bold; color: #555;")
        self.lbl_elapsed_time.setStyleSheet("font-weight: bold; color: #1976D2;")
        time_layout.addWidget(self.lbl_start_time); time_layout.addWidget(self.lbl_elapsed_time)
        dash_layout.addLayout(time_layout)

        self.lbl_trigger_rate = QLabel("Trigger Rate: 0.0 Hz")
        self.lbl_trigger_rate.setFont(QFont("Arial", 12, QFont.Bold))
        self.lbl_trigger_rate.setStyleSheet("color: #E91E63;") 
        dash_layout.addWidget(self.lbl_trigger_rate)

        dash_layout.addWidget(QLabel("Acquired Events:"))
        self.lcd_events = QLCDNumber(); self.lcd_events.setDigitCount(9)
        self.lcd_events.setStyleSheet("background-color: black; color: #00FF00;"); self.lcd_events.setMinimumHeight(40) 
        dash_layout.addWidget(self.lcd_events)
        
        dash_layout.addWidget(QLabel("Automation Progress:"))
        self.auto_progress = QProgressBar(); self.auto_progress.setAlignment(Qt.AlignCenter); self.auto_progress.setFixedHeight(20)
        dash_layout.addWidget(self.auto_progress)
        
        self.lbl_disk = QLabel("Disk Space: Calculating...")
        dash_layout.addWidget(self.lbl_disk)
        
        dash_layout.addWidget(QLabel("Current Config Summary:"))
        self.txt_config_summary = QTextEdit(); self.txt_config_summary.setReadOnly(True)
        self.txt_config_summary.setStyleSheet("background-color: #E3F2FD; color: #3E2723; font-size: 11px;")
        dash_layout.addWidget(self.txt_config_summary, stretch=1)
        
        dash_group.setLayout(dash_layout); right_panel.addWidget(dash_group, stretch=4); middle_layout.addLayout(right_panel, stretch=4)
        main_layout.addLayout(middle_layout)

        bottom_layout = QHBoxLayout()
        bottom_layout.setContentsMargins(0, 5, 0, 0)
        
        log_group = QGroupBox("System Console"); log_layout = QVBoxLayout(); log_layout.setContentsMargins(5, 5, 5, 5)
        self.lbl_hw_status = QLabel("Hardware Status: Waiting for DAQ...")
        self.lbl_hw_status.setFont(QFont("Consolas", 12, QFont.Bold))
        self.lbl_hw_status.setStyleSheet("background-color: #1A1A1A; color: #FFD740; padding: 4px; border-radius: 4px;")
        log_layout.addWidget(self.lbl_hw_status)

        self.log_viewer = QTextEdit(); self.log_viewer.setReadOnly(True)
        self.log_viewer.setStyleSheet("background-color: #121212; color: #E0E0E0; font-family: 'Consolas', 'Segoe UI Emoji', 'Apple Color Emoji', monospace; font-size: 12px;")
        log_layout.addWidget(self.log_viewer); log_group.setLayout(log_layout); bottom_layout.addWidget(log_group)
        
        main_layout.addLayout(bottom_layout, stretch=2) 
        self.refresh_configs()
        self.input_runnum.setText(self.elog_panel.get_next_run_number())

    def update_data_dir(self, new_dir):
        self.data_output_dir = new_dir
        self.print_log(f"\033[1;36m[SYSTEM]\033[0m Data output directory changed to: {self.data_output_dir}")

    def init_daq_tabs(self):
        tab_manual = QWidget(); manual_layout = QVBoxLayout(tab_manual)
        manual_layout.setContentsMargins(8, 8, 8, 8); manual_layout.setSpacing(8)
        
        self.combo_config = QComboBox()
        self.combo_config.currentIndexChanged.connect(self.update_config_summary) 
        self.input_runnum = QLineEdit(); self.input_runnum.setValidator(QRegExpValidator(QRegExp("^[0-9]+$")))
        
        self.combo_runtag = QComboBox()
        self.combo_runtag.addItems(["TEST", "PHYSICS", "CALIBRATION", "COSMIC"])
        self.combo_runtag.setStyleSheet("background-color: #FFF3E0;")
        
        self.combo_quality = QComboBox()
        self.combo_quality.addItems(["PENDING (미정)", "GOOD (정상)", "BAD (폐기)"])
        self.combo_quality.setStyleSheet("background-color: #E8EAF6;")
        
        self.input_desc = QLineEdit()
        
        manual_layout.addWidget(QLabel("Base Config File:")); manual_layout.addWidget(self.combo_config)
        
        row1 = QHBoxLayout(); row1.addWidget(QLabel("Run:")); row1.addWidget(self.input_runnum)
        row1.addWidget(QLabel("Tag:")); row1.addWidget(self.combo_runtag)
        row1.addWidget(QLabel("Quality:")); row1.addWidget(self.combo_quality)
        manual_layout.addLayout(row1)
        
        manual_layout.addWidget(QLabel("Description:")); manual_layout.addWidget(self.input_desc)
        
        stop_group = QGroupBox("Stop Condition")
        stop_layout = QHBoxLayout(); stop_layout.setContentsMargins(5, 10, 5, 5)
        self.bg_manual = QButtonGroup()
        self.rb_manual_cont = QRadioButton("Continuous (수동 정지)"); self.rb_manual_evt = QRadioButton("By Events"); self.rb_manual_time = QRadioButton("By Time (Sec)")
        self.rb_manual_cont.setChecked(True)
        self.bg_manual.addButton(self.rb_manual_cont); self.bg_manual.addButton(self.rb_manual_evt); self.bg_manual.addButton(self.rb_manual_time)
        self.spin_manual_val = QSpinBox(); self.spin_manual_val.setRange(1, 10000000); self.spin_manual_val.setValue(5000); self.spin_manual_val.setEnabled(False)
        self.rb_manual_cont.toggled.connect(lambda: self.spin_manual_val.setEnabled(not self.rb_manual_cont.isChecked()))
        
        stop_layout.addWidget(self.rb_manual_cont); stop_layout.addWidget(self.rb_manual_evt); stop_layout.addWidget(self.rb_manual_time); stop_layout.addWidget(self.spin_manual_val)
        stop_group.setLayout(stop_layout); manual_layout.addWidget(stop_group)

        btn_m = QPushButton("▶ Start Manual DAQ")
        btn_m.setStyleSheet("background-color: #4CAF50; color: white; padding: 10px; font-weight:bold; font-size: 13px;")
        btn_m.clicked.connect(self.start_manual_daq); manual_layout.addWidget(btn_m); manual_layout.addStretch()
        
        tab_scan = QWidget(); scan_layout = QVBoxLayout(tab_scan)
        scan_layout.setContentsMargins(8, 8, 8, 8); scan_layout.setSpacing(6)
        scan_layout.addWidget(QLabel("자동으로 Threshold(THR) 값을 변경하며 스캔합니다."))
        row2 = QHBoxLayout()
        self.spin_thr_start = QSpinBox(); self.spin_thr_start.setRange(0, 1000); self.spin_thr_start.setValue(50)
        self.spin_thr_end = QSpinBox(); self.spin_thr_end.setRange(0, 1000); self.spin_thr_end.setValue(200)
        self.spin_thr_step = QSpinBox(); self.spin_thr_step.setRange(1, 500); self.spin_thr_step.setValue(10)
        row2.addWidget(QLabel("Start:")); row2.addWidget(self.spin_thr_start); row2.addWidget(QLabel("End:")); row2.addWidget(self.spin_thr_end); row2.addWidget(QLabel("Step:")); row2.addWidget(self.spin_thr_step); scan_layout.addLayout(row2)
        self.bg_scan = QButtonGroup()
        self.rb_scan_evt = QRadioButton("By Events"); self.rb_scan_time = QRadioButton("By Time (Sec)"); self.rb_scan_evt.setChecked(True)
        self.bg_scan.addButton(self.rb_scan_evt); self.bg_scan.addButton(self.rb_scan_time)
        row_scan_opt = QHBoxLayout(); row_scan_opt.addWidget(self.rb_scan_evt); row_scan_opt.addWidget(self.rb_scan_time)
        self.spin_scan_val = QSpinBox(); self.spin_scan_val.setRange(1, 10000000); self.spin_scan_val.setValue(5000)
        row_scan_opt.addWidget(self.spin_scan_val); scan_layout.addLayout(row_scan_opt)
        btn_s = QPushButton("🔄 Start Threshold Scan")
        btn_s.setStyleSheet("background-color: #FF9800; color: white; padding: 10px; font-weight:bold;")
        btn_s.clicked.connect(self.start_thr_scan); scan_layout.addWidget(btn_s); scan_layout.addStretch()

        tab_subrun = QWidget(); subrun_layout = QVBoxLayout(tab_subrun)
        subrun_layout.setContentsMargins(8, 8, 8, 8); subrun_layout.setSpacing(6)
        subrun_layout.addWidget(QLabel("지정된 조건(시간/이벤트) 단위로 파일을 분할 수집합니다."))
        self.bg_long = QButtonGroup()
        self.rb_long_time = QRadioButton("Chunk by Time (Min)"); self.rb_long_evt = QRadioButton("Chunk by Events"); self.rb_long_time.setChecked(True)
        self.bg_long.addButton(self.rb_long_time); self.bg_long.addButton(self.rb_long_evt)
        row3 = QHBoxLayout(); row3.addWidget(self.rb_long_time); row3.addWidget(self.rb_long_evt); subrun_layout.addLayout(row3)
        row4 = QHBoxLayout()
        self.spin_chunk_val = QSpinBox(); self.spin_chunk_val.setRange(1, 10000000); self.spin_chunk_val.setValue(60)
        self.spin_total_chunk = QSpinBox(); self.spin_total_chunk.setRange(1, 1000); self.spin_total_chunk.setValue(10)
        row4.addWidget(QLabel("Value/Chunk:")); row4.addWidget(self.spin_chunk_val); row4.addWidget(QLabel("Total Chunks:")); row4.addWidget(self.spin_total_chunk); subrun_layout.addLayout(row4)
        btn_sub = QPushButton("⏱️ Start Long Run")
        btn_sub.setStyleSheet("background-color: #009688; color: white; padding: 10px; font-weight:bold;")
        btn_sub.clicked.connect(self.start_subrun); subrun_layout.addWidget(btn_sub); subrun_layout.addStretch()

        self.daq_tabs.addTab(tab_manual, "🕹️ Manual DAQ")
        self.daq_tabs.addTab(tab_scan, "🔄 THR Scan")
        self.daq_tabs.addTab(tab_subrun, "⏱️ Long Run")

    def update_config_summary(self):
        path = self.combo_config.currentData()
        if not path or not os.path.exists(path): return
        summary = ""
        with open(path, 'r') as f:
            for line in f:
                if line.strip().startswith("FADC"): summary += f"[보드] {line.strip()}\n"
                elif line.strip().startswith("NDP"): summary += f"[포인트] {line.strip()}\n"
                elif line.strip().startswith("TLT"): summary += f"[트리거] {line.strip()}\n"
                elif line.strip().startswith("THR"): summary += f"[임계값] {line.strip()}\n"
                elif line.strip().startswith("DLY"): summary += f"[지연] {line.strip()}\n"
        self.txt_config_summary.setText(summary)

    def start_monitor(self):
        if self.monitor_process.state() != QProcess.Running:
            self.monitor_process.start(os.path.join(CURRENT_DIR, EXE_MONITOR), [])
            self.print_log("\033[1;36m[MONITOR]\033[0m Online Display Process Started.")
            
    def stop_monitor(self):
        if self.monitor_process.state() == QProcess.Running:
            self.monitor_process.terminate()
            self.print_log("\033[1;36m[MONITOR]\033[0m Online Display stopped.")

    def update_clock(self):
        self.clock_lbl.setText(datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
        total, used, free = shutil.disk_usage(self.data_output_dir)
        gb_free = free // (2**30)
        self.lbl_disk.setText(f"Disk Free [{os.path.basename(self.data_output_dir)}]: {gb_free} GB")
        self.lbl_disk.setStyleSheet("color: red; font-weight: bold;" if gb_free < 5 else "color: black;")
        
        if self.daq_process.state() == QProcess.Running and self.daq_start_timestamp:
            elapsed = datetime.now() - self.daq_start_timestamp
            hours, remainder = divmod(elapsed.seconds, 3600)
            minutes, seconds = divmod(remainder, 60)
            self.lbl_elapsed_time.setText(f"Elapsed: {hours:02d}:{minutes:02d}:{seconds:02d}")

    def refresh_configs(self):
        self.combo_config.clear()
        if os.path.exists(CONFIG_DIR):
            for cfg in glob.glob(os.path.join(CONFIG_DIR, "*.config")): self.combo_config.addItem(os.path.basename(cfg), cfg)
        self.update_config_summary()

    def ansi_to_html(self, text):
        text = text.replace('<', '&lt;').replace('>', '&gt;')
        text = re.sub(r'\033\[1;31m(.*?)\033\[0m', r'<span style="color:#FF5252; font-weight:bold;">\1</span>', text) 
        text = re.sub(r'\033\[1;32m(.*?)\033\[0m', r'<span style="color:#69F0AE; font-weight:bold;">\1</span>', text) 
        text = re.sub(r'\033\[1;33m(.*?)\033\[0m', r'<span style="color:#FFD740; font-weight:bold;">\1</span>', text) 
        text = re.sub(r'\033\[1;34m(.*?)\033\[0m', r'<span style="color:#448AFF; font-weight:bold;">\1</span>', text) 
        text = re.sub(r'\033\[1;35m(.*?)\033\[0m', r'<span style="color:#E040FB; font-weight:bold;">\1</span>', text) 
        text = re.sub(r'\033\[1;36m(.*?)\033\[0m', r'<span style="color:#18FFFF; font-weight:bold;">\1</span>', text) 
        text = re.sub(r'\033\[1;37m(.*?)\033\[0m', r'<span style="color:#FFFFFF; font-weight:bold;">\1</span>', text) 
        text = re.sub(r'\033\[1;41m(.*?)\033\[0m', r'<span style="color:#FFFFFF; background-color:#FF5252; font-weight:bold; padding:2px;">\1</span>', text) 
        text = re.sub(r'\033\[[\d;]*m', '', text) 
        return text

    def print_log(self, text, is_error=False):
        for line in text.splitlines():
            if not line.strip(): continue
            if is_error:
                clean_line = re.sub(r'\033\[[\d;]*m', '', line)
                safe_line = clean_line.replace('<', '&lt;').replace('>', '&gt;')
                self.log_viewer.append(f'<span style="color:#FF5252; font-weight:bold;">{safe_line}</span>')
            else:
                html_line = self.ansi_to_html(line)
                self.log_viewer.append(html_line)
        self.log_viewer.moveCursor(QTextCursor.End)

    # [핵심] 모니터 로그 가로채기
    def handle_mon_stdout(self):
        raw_data = self.monitor_process.readAllStandardOutput().data().decode("utf8", errors="replace")
        for line in raw_data.split('\n'):
            for subline in line.split('\r'):
                if subline.strip():
                    self.print_log(f"\033[1;34m[MONITOR]\033[0m {subline.strip()}")

    def handle_mon_stderr(self):
        raw_data = self.monitor_process.readAllStandardError().data().decode("utf8", errors="replace")
        for line in raw_data.split('\n'):
            for subline in line.split('\r'):
                if subline.strip():
                    self.print_log(f"\033[1;31m[MON ERROR]\033[0m {subline.strip()}", is_error=True)

    def generate_scan_config(self, base_cfg, target_thr):
        with open(base_cfg, 'r') as f: lines = f.readlines()
        with open(TEMP_CONFIG, 'w') as f:
            for line in lines:
                if line.strip().startswith("THR"):
                    parts = line.split()
                    f.write(f"THR    {parts[1]}    " + "    ".join([str(target_thr)] * (len(parts)-2)) + "\n")
                else: f.write(line)
        return TEMP_CONFIG

    def pre_start_check(self):
        if not self.input_runnum.text().strip(): QMessageBox.warning(self, "Warning", "Run Number를 입력하세요!"); return False
        if not self.combo_config.currentData(): QMessageBox.warning(self, "Warning", "Config 파일을 선택하세요!"); return False
        free = shutil.disk_usage(self.data_output_dir)[2] // (2**30)
        if free < 2: QMessageBox.critical(self, "Disk Full", f"해당 디스크({self.data_output_dir}) 용량이 2GB 미만입니다!"); return False
        
        self.daq_start_timestamp = datetime.now()
        self.lbl_start_time.setText(f"Start: {self.daq_start_timestamp.strftime('%H:%M:%S')}")
        self.lbl_elapsed_time.setText("Elapsed: 00:00:00")
        self.lbl_trigger_rate.setText("Trigger Rate: 0.0 Hz")
        
        self.final_events = "0"; self.final_time = "0.0"; self.final_rate = "0.0"
        
        self.tabs.setEnabled(False); self.log_viewer.clear(); self.lcd_events.display(0)
        return True

    def trigger_manual_timeout(self):
        self.manual_timer.stop()
        self.print_log("\033[1;36m[AUTO]\033[0m Manual DAQ target time reached. Graceful shutdown...", is_error=True)
        self.daq_process.terminate()

    def start_manual_daq(self):
        if not self.pre_start_check(): return
        self.auto_mode = "NONE"; run_num = self.input_runnum.text().strip()
        self.lbl_dash_mode.setText(f"MANUAL [{run_num}]")
        
        run_tag = self.combo_runtag.currentText()
        quality = self.combo_quality.currentText().split()[0]
        self.current_run_id = self.elog_panel.record_run(run_num, "MANUAL", run_tag, quality, self.input_desc.text())
        
        out_root = os.path.join(self.data_output_dir, f"run_{run_num}.root") 
        args = ["-f", self.combo_config.currentData(), "-o", out_root]
        
        if self.rb_manual_evt.isChecked():
            val = self.spin_manual_val.value(); args.extend(["-n", str(val)])
            self.print_log(f"\033[1;36m[SYSTEM]\033[0m Manual DAQ will stop after {val} events.")
            self.daq_process.start(os.path.join(CURRENT_DIR, EXE_FRONTEND), args)
        elif self.rb_manual_time.isChecked():
            val = self.spin_manual_val.value()
            self.print_log(f"\033[1;36m[SYSTEM]\033[0m Manual DAQ will stop after {val} seconds.")
            self.daq_process.start(os.path.join(CURRENT_DIR, EXE_FRONTEND), args)
            self.manual_timer.start(val * 1000)
        else:
            self.print_log(f"\033[1;36m[SYSTEM]\033[0m Manual DAQ running continuously. (Press ABORT to end)")
            self.daq_process.start(os.path.join(CURRENT_DIR, EXE_FRONTEND), args)

    def start_thr_scan(self):
        if not self.pre_start_check(): return
        self.auto_mode = "SCAN"; self.current_base_runnum = self.input_runnum.text().strip()
        self.scan_queue = list(range(self.spin_thr_start.value(), self.spin_thr_end.value() + 1, self.spin_thr_step.value()))
        if not self.scan_queue: return
        self.auto_progress.setMaximum(len(self.scan_queue)); self.auto_progress.setValue(0)
        self.run_next_scan_step()

    def run_next_scan_step(self):
        if not self.scan_queue: self.auto_finish(); return
        self.scan_current_val = self.scan_queue.pop(0)
        self.auto_progress.setValue(self.auto_progress.maximum() - len(self.scan_queue))
        cfg_path = self.generate_scan_config(self.combo_config.currentData(), self.scan_current_val)
        run_str = f"{self.current_base_runnum}_THR{self.scan_current_val}"
        self.lbl_dash_mode.setText(f"SCAN [THR={self.scan_current_val}]")
        
        self.current_run_id = self.elog_panel.record_run(run_str, "SCAN", "CALIBRATION", "PENDING", f"Auto Scan: THR={self.scan_current_val}")
        out_root = os.path.join(self.data_output_dir, f"run_{run_str}.root") 
        
        args = ["-f", cfg_path, "-o", out_root]; val = self.spin_scan_val.value()
        if self.rb_scan_evt.isChecked():
            args.extend(["-n", str(val)]); self.print_log(f"\n\033[1;36m[AUTO]\033[0m Scan Step: THR={self.scan_current_val} ({val} Events)")
            self.daq_process.start(os.path.join(CURRENT_DIR, EXE_FRONTEND), args)
        else:
            self.print_log(f"\n\033[1;36m[AUTO]\033[0m Scan Step: THR={self.scan_current_val} ({val} Sec timeout)")
            self.daq_process.start(os.path.join(CURRENT_DIR, EXE_FRONTEND), args)
            self.scan_timer.start(val * 1000)

    def trigger_scan_timeout(self):
        self.scan_timer.stop()
        self.print_log("\033[1;36m[AUTO]\033[0m Scan time reached. Force saving...", is_error=True)
        self.daq_process.terminate()

    def start_subrun(self):
        if not self.pre_start_check(): return
        self.auto_mode = "SUBRUN"; self.current_base_runnum = self.input_runnum.text().strip()
        self.current_subrun_idx = 1; self.subrun_max_idx = self.spin_total_chunk.value()
        self.auto_progress.setMaximum(self.subrun_max_idx); self.auto_progress.setValue(0)
        self.run_next_subrun()

    def run_next_subrun(self):
        if self.current_subrun_idx > self.subrun_max_idx: self.auto_finish(); return
        self.auto_progress.setValue(self.current_subrun_idx)
        run_str = f"{self.current_base_runnum}_part{self.current_subrun_idx:02d}"
        self.lbl_dash_mode.setText(f"LONG RUN [{self.current_subrun_idx}/{self.subrun_max_idx}]")
        
        self.current_run_id = self.elog_panel.record_run(run_str, "LONG_RUN", "PHYSICS", "PENDING", f"Chunk {self.current_subrun_idx}/{self.subrun_max_idx}")
        out_root = os.path.join(self.data_output_dir, f"run_{run_str}.root")
        
        args = ["-f", self.combo_config.currentData(), "-o", out_root]; val = self.spin_chunk_val.value()
        if self.rb_long_evt.isChecked():
            args.extend(["-n", str(val)]); self.print_log(f"\n\033[1;36m[AUTO]\033[0m Long Run Chunk {self.current_subrun_idx} ({val} Events)...")
            self.daq_process.start(os.path.join(CURRENT_DIR, EXE_FRONTEND), args)
        else:
            self.print_log(f"\n\033[1;36m[AUTO]\033[0m Long Run Chunk {self.current_subrun_idx} ({val} Min timeout)...")
            self.daq_process.start(os.path.join(CURRENT_DIR, EXE_FRONTEND), args)
            self.subrun_timer.start(val * 60 * 1000)

    def trigger_subrun_rotation(self):
        self.subrun_timer.stop()
        self.print_log("\033[1;36m[AUTO]\033[0m Chunk time reached. Rotating...", is_error=True)
        self.daq_process.terminate() 

    def force_stop_daq(self):
        self.auto_mode = "NONE"; self.scan_queue.clear()
        self.subrun_timer.stop(); self.scan_timer.stop(); self.manual_timer.stop()
        
        if self.daq_process.state() == QProcess.Running:
            self.print_log("\033[1;31m[SYSTEM] ABORTING DAQ gracefully...\033[0m", is_error=True)
            self.daq_process.terminate()
        self.hv_panel.force_shutdown()

    def auto_finish(self):
        self.auto_mode = "NONE"; self.lbl_dash_mode.setText("IDLE"); self.tabs.setEnabled(True)
        self.input_runnum.setText(self.elog_panel.get_next_run_number()) 
        self.print_log("\n\033[1;32m[SYSTEM]\033[0m === Sequences Completed Successfully! ===")
        self.lbl_trigger_rate.setText("Trigger Rate: 0.0 Hz")
        self.daq_start_timestamp = None

    def handle_stdout(self):
        raw_data = self.daq_process.readAllStandardOutput().data().decode("utf8", errors="replace")
        for line in raw_data.split('\n'):
            for subline in line.split('\r'): 
                if not subline.strip(): continue
                clean_line = re.sub(r'\033\[[\d;]*m', '', subline)
                
                match_rate = re.search(r'Rate:\s*([0-9.]+)\s*Hz', clean_line)
                if match_rate:
                    self.lbl_trigger_rate.setText(f"Trigger Rate: {match_rate.group(1)} Hz")
                    self.final_rate = match_rate.group(1)
                
                match_time = re.search(r'Time:\s*([0-9.]+)\s*s', clean_line)
                if match_time: self.final_time = match_time.group(1)
                
                match_ev = re.search(r'Events:\s*(\d+)', clean_line)
                if match_ev: 
                    self.lcd_events.display(int(match_ev.group(1)))
                    self.final_events = match_ev.group(1)
                    self.lbl_hw_status.setText(self.ansi_to_html(subline))
                    continue 

                match_sum_ev = re.search(r'Total Events\s*:\s*(\d+)', clean_line)
                if match_sum_ev: self.final_events = match_sum_ev.group(1)
                
                match_sum_time = re.search(r'Total Elapsed Time\s*:\s*([0-9.]+)\s*sec', clean_line)
                if match_sum_time: self.final_time = match_sum_time.group(1)
                
                match_sum_rate = re.search(r'Average Trigger Rate\s*:\s*([0-9.]+)\s*Hz', clean_line)
                if match_sum_rate: self.final_rate = match_sum_rate.group(1)

                self.print_log(subline)

    def handle_stderr(self):
        self.print_log(self.daq_process.readAllStandardError().data().decode("utf8", errors="replace"), is_error=True)

    def process_finished(self):
        if hasattr(self, 'current_run_id'):
            status_str = "COMPLETED"
            if hasattr(self, 'final_events') and self.final_events != "0":
                status_str = f"COMPLETED | {self.final_events} evts | {self.final_time}s | {self.final_rate}Hz"
            
            self.elog_panel.update_status(self.current_run_id, status_str)
        
        self.lbl_hw_status.setText("Hardware Status: Waiting for DAQ...")
        
        if self.auto_mode == "SCAN": QTimer.singleShot(1000, self.run_next_scan_step)
        elif self.auto_mode == "SUBRUN": self.current_subrun_idx += 1; QTimer.singleShot(1000, self.run_next_subrun)
        elif self.auto_mode == "NONE": self.auto_finish()

    def closeEvent(self, event):
        if self.daq_process.state() == QProcess.Running or (hasattr(self.hv_panel, 'device') and self.hv_panel.device is not None):
            reply = QMessageBox.question(self, 'System Alert', "DAQ or HV active. Stop and exit?", QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if reply == QMessageBox.Yes:
                self.force_stop_daq()
                if self.daq_process.state() == QProcess.Running: self.daq_process.waitForFinished(3000) 
                if self.monitor_process.state() == QProcess.Running: self.monitor_process.terminate()
                self.prod_panel.terminate_process() 
                event.accept()
            else: event.ignore()
        else:
            if self.monitor_process.state() == QProcess.Running: self.monitor_process.terminate()
            self.prod_panel.terminate_process() 
            event.accept()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = DAQControlCenter()
    window.show()
    sys.exit(app.exec_())