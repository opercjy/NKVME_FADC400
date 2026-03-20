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
                             QTableWidget, QTableWidgetItem, QGroupBox, QHeaderView, 
                             QMessageBox, QTextEdit, QComboBox, QCheckBox, 
                             QLCDNumber, QProgressBar, QTabWidget, QSpinBox)
from PyQt5.QtCore import QProcess, Qt, QRegExp, QTimer
from PyQt5.QtGui import QRegExpValidator, QColor, QTextCursor, QFont

# ==========================================================
# 1. 경로 및 DB 설정
# ==========================================================
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(CURRENT_DIR) 

DB_NAME = os.path.join(PROJECT_ROOT, "daq_history.db")
CONFIG_DIR = os.path.join(PROJECT_ROOT, "config")
TEMP_CONFIG = os.path.join(CONFIG_DIR, "temp_auto.config") # 자동화용 임시 설정파일

def init_db():
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS run_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            run_num TEXT,
            run_type TEXT,
            description TEXT,
            start_time TEXT,
            status TEXT
        )
    ''')
    conn.commit()
    conn.close()

# ==========================================================
# 2. 메인 통제실 GUI 클래스 (Auto-Sequencer 내장)
# ==========================================================
class DAQControlCenter(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("NoticeDAQ Slow Control & Auto-Sequencer")
        self.resize(1150, 850)
        
        # 프로세스 제어
        self.daq_process = QProcess(self)
        self.daq_process.readyReadStandardOutput.connect(self.handle_stdout)
        self.daq_process.readyReadStandardError.connect(self.handle_stderr)
        self.daq_process.finished.connect(self.process_finished)
        self.monitor_process = QProcess(self)

        # 자동화(Automation) 상태 머신 변수
        self.auto_mode = "NONE" # NONE, SCAN, SUBRUN
        self.scan_queue = []
        self.scan_current_val = 0
        self.subrun_timer = QTimer(self)
        self.subrun_timer.timeout.connect(self.trigger_subrun_rotation)
        self.current_base_runnum = ""
        self.current_subrun_idx = 1
        self.subrun_max_idx = 1

        self.init_ui()
        self.load_history()

        # 시계
        self.clock_timer = QTimer(self)
        self.clock_timer.timeout.connect(self.update_clock)
        self.clock_timer.start(1000)

    def init_ui(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)

        # --- TOP: 타이틀 및 시계 ---
        top_layout = QHBoxLayout()
        title_lbl = QLabel("NoticeDAQ Slow Control System")
        title_lbl.setFont(QFont("Arial", 16, QFont.Bold))
        self.clock_lbl = QLabel("0000-00-00 00:00:00")
        self.clock_lbl.setFont(QFont("Consolas", 14, QFont.Bold))
        self.clock_lbl.setStyleSheet("color: #2196F3;")
        top_layout.addWidget(title_lbl)
        top_layout.addStretch()
        top_layout.addWidget(self.clock_lbl)
        main_layout.addLayout(top_layout)

        # --- MIDDLE: 제어(좌) vs 대시보드(우) ---
        middle_layout = QHBoxLayout()

        # [좌측 패널] 탭(Tab) 구조 적용
        left_panel = QVBoxLayout()
        self.tabs = QTabWidget()
        
        # 탭 1: 수동 제어 (Manual)
        tab_manual = QWidget()
        manual_layout = QVBoxLayout(tab_manual)
        
        # 공통 설정 콤보
        self.combo_config = QComboBox()
        self.refresh_configs()
        manual_layout.addWidget(QLabel("Base Config File:"))
        manual_layout.addWidget(self.combo_config)
        
        row_run = QHBoxLayout()
        self.input_runnum = QLineEdit()
        self.input_runnum.setValidator(QRegExpValidator(QRegExp("^[0-9]+$")))
        self.input_runnum.setPlaceholderText("ex) 101")
        self.input_runtype = QLineEdit("TEST")
        row_run.addWidget(QLabel("Run Num:"))
        row_run.addWidget(self.input_runnum)
        row_run.addWidget(QLabel("Type:"))
        row_run.addWidget(self.input_runtype)
        manual_layout.addLayout(row_run)
        
        row_desc = QHBoxLayout()
        self.input_desc = QLineEdit()
        self.input_desc.setPlaceholderText("Run Description...")
        row_desc.addWidget(QLabel("Desc:"))
        row_desc.addWidget(self.input_desc, stretch=1)
        manual_layout.addLayout(row_desc)

        btn_manual_start = QPushButton("▶ Start Single DAQ")
        btn_manual_start.setStyleSheet("background-color: #4CAF50; color: white; padding: 10px; font-weight: bold;")
        btn_manual_start.clicked.connect(self.start_manual_daq)
        manual_layout.addWidget(btn_manual_start)
        manual_layout.addStretch()

        # 탭 2: 문턱값 스캔 (Threshold Scan)
        tab_scan = QWidget()
        scan_layout = QVBoxLayout(tab_scan)
        scan_layout.addWidget(QLabel("자동으로 Threshold(THR) 값을 변경하며 데이터를 수집합니다."))
        
        row_scan = QHBoxLayout()
        self.spin_thr_start = QSpinBox(); self.spin_thr_start.setRange(0, 1000); self.spin_thr_start.setValue(50)
        self.spin_thr_end = QSpinBox(); self.spin_thr_end.setRange(0, 1000); self.spin_thr_end.setValue(200)
        self.spin_thr_step = QSpinBox(); self.spin_thr_step.setRange(1, 500); self.spin_thr_step.setValue(10)
        row_scan.addWidget(QLabel("Start:")); row_scan.addWidget(self.spin_thr_start)
        row_scan.addWidget(QLabel("End:")); row_scan.addWidget(self.spin_thr_end)
        row_scan.addWidget(QLabel("Step:")); row_scan.addWidget(self.spin_thr_step)
        scan_layout.addLayout(row_scan)
        
        row_scan_evt = QHBoxLayout()
        self.input_scan_evt = QLineEdit("5000") # 스텝당 5000 이벤트
        row_scan_evt.addWidget(QLabel("Events per Step:"))
        row_scan_evt.addWidget(self.input_scan_evt)
        scan_layout.addLayout(row_scan_evt)

        btn_scan_start = QPushButton("🔄 Start Threshold Scan")
        btn_scan_start.setStyleSheet("background-color: #FF9800; color: white; padding: 10px; font-weight: bold;")
        btn_scan_start.clicked.connect(self.start_thr_scan)
        scan_layout.addWidget(btn_scan_start)
        scan_layout.addStretch()

        # 탭 3: 장기 런 자동 분할 (Sub-run Chunking)
        tab_subrun = QWidget()
        subrun_layout = QVBoxLayout(tab_subrun)
        subrun_layout.addWidget(QLabel("장기간 데이터를 안전하게 받기 위해 시간 단위로 파일을 분할합니다."))
        
        row_sub = QHBoxLayout()
        self.spin_chunk_min = QSpinBox(); self.spin_chunk_min.setRange(1, 1440); self.spin_chunk_min.setValue(60)
        self.spin_total_chunk = QSpinBox(); self.spin_total_chunk.setRange(1, 100); self.spin_total_chunk.setValue(10)
        row_sub.addWidget(QLabel("Chunk Time (Min):")); row_sub.addWidget(self.spin_chunk_min)
        row_sub.addWidget(QLabel("Total Chunks:")); row_sub.addWidget(self.spin_total_chunk)
        subrun_layout.addLayout(row_sub)
        
        btn_sub_start = QPushButton("⏱️ Start Long Run (Chunking)")
        btn_sub_start.setStyleSheet("background-color: #009688; color: white; padding: 10px; font-weight: bold;")
        btn_sub_start.clicked.connect(self.start_subrun)
        subrun_layout.addWidget(btn_sub_start)
        subrun_layout.addStretch()

        self.tabs.addTab(tab_manual, "Manual")
        self.tabs.addTab(tab_scan, "THR Scan")
        self.tabs.addTab(tab_subrun, "Long Run")
        left_panel.addWidget(self.tabs)

        # 공용 제어 (Stop, Monitor)
        self.cb_auto_monitor = QCheckBox("자동으로 실시간 GUI 모니터 켜기")
        self.cb_auto_monitor.setChecked(True)
        left_panel.addWidget(self.cb_auto_monitor)

        self.btn_stop = QPushButton("■ FORCE STOP ALL")
        self.btn_stop.setStyleSheet("background-color: #F44336; color: white; font-weight: bold; padding: 15px;")
        self.btn_stop.setEnabled(False)
        self.btn_stop.clicked.connect(self.force_stop_daq)
        left_panel.addWidget(self.btn_stop)

        middle_layout.addLayout(left_panel, stretch=1)

        # [우측 패널] 라이브 대시보드
        right_panel = QVBoxLayout()
        dash_group = QGroupBox("Live Status Dashboard")
        dash_group.setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #9E9E9E; }")
        dash_layout = QVBoxLayout()

        self.lbl_dash_mode = QLabel("IDLE")
        self.lbl_dash_mode.setFont(QFont("Arial", 16, QFont.Bold))
        self.lbl_dash_mode.setStyleSheet("color: #FF9800;")
        dash_layout.addWidget(QLabel("Current Mode:")); dash_layout.addWidget(self.lbl_dash_mode)

        dash_layout.addWidget(QLabel("Acquired Events (Current Step):"))
        self.lcd_events = QLCDNumber(); self.lcd_events.setDigitCount(9)
        self.lcd_events.setStyleSheet("background-color: black; color: #00FF00;")
        self.lcd_events.setMinimumHeight(50)
        dash_layout.addWidget(self.lcd_events)

        dash_layout.addWidget(QLabel("Automation Progress:"))
        self.auto_progress = QProgressBar()
        self.auto_progress.setValue(0)
        self.auto_progress.setAlignment(Qt.AlignCenter)
        dash_layout.addWidget(self.auto_progress)

        # 디스크 용량 모니터링
        self.lbl_disk = QLabel("Disk Space: Calculating...")
        dash_layout.addWidget(self.lbl_disk)

        dash_group.setLayout(dash_layout)
        right_panel.addWidget(dash_group)
        middle_layout.addLayout(right_panel, stretch=1)

        main_layout.addLayout(middle_layout)

        # --- BOTTOM: DB 테이블 및 로그 ---
        bottom_layout = QHBoxLayout()
        db_group = QGroupBox("Run History")
        db_layout = QVBoxLayout()
        self.table = QTableWidget(0, 6)
        self.table.setHorizontalHeaderLabels(["ID", "Run", "Type", "Description", "Start Time", "Status"])
        db_layout.addWidget(self.table)
        db_group.setLayout(db_layout)
        bottom_layout.addWidget(db_group, stretch=5)

        log_group = QGroupBox("System Console")
        log_layout = QVBoxLayout()
        self.log_viewer = QTextEdit()
        self.log_viewer.setReadOnly(True)
        self.log_viewer.setStyleSheet("background-color: #1E1E1E; color: #00FF00; font-family: monospace;")
        log_layout.addWidget(self.log_viewer)
        log_group.setLayout(log_layout)
        bottom_layout.addWidget(log_group, stretch=4)

        main_layout.addLayout(bottom_layout)

    # ==========================================================
    # 3. 유틸리티 및 모니터링 로직
    # ==========================================================
    def update_clock(self):
        self.clock_lbl.setText(datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
        # 디스크 용량 업데이트 (1초마다)
        total, used, free = shutil.disk_usage("/")
        gb_free = free // (2**30)
        self.lbl_disk.setText(f"Disk Free Space: {gb_free} GB")
        if gb_free < 5:
            self.lbl_disk.setStyleSheet("color: red; font-weight: bold;")

    def refresh_configs(self):
        self.combo_config.clear()
        if os.path.exists(CONFIG_DIR):
            for cfg in glob.glob(os.path.join(CONFIG_DIR, "*.config")):
                self.combo_config.addItem(os.path.basename(cfg), cfg)

    def load_history(self):
        self.table.setRowCount(0)
        conn = sqlite3.connect(DB_NAME)
        cur = conn.cursor()
        cur.execute("SELECT * FROM run_history ORDER BY id DESC LIMIT 50")
        for row_idx, row_data in enumerate(cur.fetchall()):
            self.table.insertRow(row_idx)
            for col_idx, col_data in enumerate(row_data):
                item = QTableWidgetItem(str(col_data))
                if col_idx == 5: 
                    if "RUNNING" in str(col_data): item.setForeground(QColor("red"))
                    elif "COMPLETED" in str(col_data): item.setForeground(QColor("green"))
                self.table.setItem(row_idx, col_idx, item)
        conn.close()

    def print_log(self, text, is_error=False):
        color = "red" if is_error else "#00FF00"
        for line in text.splitlines():
            if not line.strip(): continue
            self.log_viewer.append(f'<span style="color:{color};">{line}</span>')
        self.log_viewer.moveCursor(QTextCursor.End)

    def record_run_db(self, run_str, run_type, desc):
        start_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        conn = sqlite3.connect(DB_NAME)
        cur = conn.cursor()
        cur.execute("INSERT INTO run_history (run_num, run_type, description, start_time, status) VALUES (?, ?, ?, ?, ?)",
                       (run_str, run_type, desc, start_time, "RUNNING"))
        self.current_run_id = cur.lastrowid
        conn.commit()
        conn.close()
        self.load_history()

    # ==========================================================
    # 4. 자동화 제어 로직 (동적 Config 생성)
    # ==========================================================
    def generate_scan_config(self, base_cfg, target_thr):
        """정규식을 이용해 기존 Config의 THR 값을 덮어씌운 임시 Config 생성"""
        with open(base_cfg, 'r') as f: lines = f.readlines()
        
        with open(TEMP_CONFIG, 'w') as f:
            for line in lines:
                if line.strip().startswith("THR"):
                    # THR 라인을 찾아 설정된 값으로 모든 채널 변경 (방어적 1채널~4채널 대응)
                    parts = line.split()
                    new_line = f"THR    {parts[1]}    " + "    ".join([str(target_thr)] * (len(parts)-2)) + "\n"
                    f.write(new_line)
                else:
                    f.write(line)
        return TEMP_CONFIG

    def pre_start_check(self):
        if not self.input_runnum.text().strip():
            QMessageBox.warning(self, "Warning", "Run Number를 입력하세요!")
            return False
        if not self.combo_config.currentData():
            QMessageBox.warning(self, "Warning", "Base Config 파일을 선택하세요!")
            return False
        
        total, used, free = shutil.disk_usage("/")
        if free // (2**30) < 2:
            QMessageBox.critical(self, "Disk Full", "디스크 용량이 2GB 미만입니다! 데이터 획득을 금지합니다.")
            return False

        if self.cb_auto_monitor.isChecked() and self.monitor_process.state() != QProcess.Running:
            self.monitor_process.start(os.path.join(CURRENT_DIR, "OnlineMonitor"), [])
        
        self.btn_stop.setEnabled(True)
        self.tabs.setEnabled(False) # 실행 중 탭 조작 금지
        self.log_viewer.clear()
        return True

    # --- 탭 1: 수동 실행 ---
    def start_manual_daq(self):
        if not self.pre_start_check(): return
        self.auto_mode = "NONE"
        run_num = self.input_runnum.text().strip()
        self.lbl_dash_mode.setText(f"MANUAL RUN [{run_num}]")
        self.record_run_db(run_num, self.input_runtype.text(), self.input_desc.text())
        
        out_root = os.path.join(PROJECT_ROOT, f"run_{run_num}.root")
        self.print_log(f"[SYSTEM] Starting Manual DAQ -> {out_root}")
        self.daq_process.start(os.path.join(CURRENT_DIR, "frontend"), ["-f", self.combo_config.currentData(), "-o", out_root])

    # --- 탭 2: 스캔 실행 ---
    def start_thr_scan(self):
        if not self.pre_start_check(): return
        self.auto_mode = "SCAN"
        self.current_base_runnum = self.input_runnum.text().strip()
        
        s_start, s_end, s_step = self.spin_thr_start.value(), self.spin_thr_end.value(), self.spin_thr_step.value()
        self.scan_queue = list(range(s_start, s_end + 1, s_step))
        
        if not self.scan_queue: return
        self.auto_progress.setMaximum(len(self.scan_queue))
        self.auto_progress.setValue(0)
        
        self.run_next_scan_step()

    def run_next_scan_step(self):
        if not self.scan_queue:
            self.auto_finish()
            return
        
        self.scan_current_val = self.scan_queue.pop(0)
        self.auto_progress.setValue(self.auto_progress.maximum() - len(self.scan_queue))
        
        # 동적 Config 생성
        cfg_path = self.generate_scan_config(self.combo_config.currentData(), self.scan_current_val)
        run_str = f"{self.current_base_runnum}_THR{self.scan_current_val}"
        
        self.lbl_dash_mode.setText(f"SCAN MODE [THR={self.scan_current_val}]")
        self.record_run_db(run_str, "SCAN", f"Auto Scan Step: THR={self.scan_current_val}")
        
        out_root = os.path.join(PROJECT_ROOT, f"run_{run_str}.root")
        events = self.input_scan_evt.text().strip()
        
        self.print_log(f"\n[AUTO] Starting Scan Step: THR={self.scan_current_val} ({events} events)")
        self.daq_process.start(os.path.join(CURRENT_DIR, "frontend"), ["-f", cfg_path, "-o", out_root, "-n", events])

    # --- 탭 3: 장기 런(Sub-run) 분할 실행 ---
    def start_subrun(self):
        if not self.pre_start_check(): return
        self.auto_mode = "SUBRUN"
        self.current_base_runnum = self.input_runnum.text().strip()
        self.current_subrun_idx = 1
        self.subrun_max_idx = self.spin_total_chunk.value()
        
        self.auto_progress.setMaximum(self.subrun_max_idx)
        self.auto_progress.setValue(0)
        
        self.run_next_subrun()

    def run_next_subrun(self):
        if self.current_subrun_idx > self.subrun_max_idx:
            self.auto_finish()
            return

        self.auto_progress.setValue(self.current_subrun_idx)
        run_str = f"{self.current_base_runnum}_part{self.current_subrun_idx:02d}"
        
        self.lbl_dash_mode.setText(f"LONG RUN [{self.current_subrun_idx}/{self.subrun_max_idx}]")
        self.record_run_db(run_str, "LONG_RUN", f"Chunk {self.current_subrun_idx} of {self.subrun_max_idx}")
        
        out_root = os.path.join(PROJECT_ROOT, f"run_{run_str}.root")
        self.print_log(f"\n[AUTO] Starting Sub-run Chunk {self.current_subrun_idx}...")
        
        # 프로세스 실행 후 지정된 시간(분) 뒤에 강제 종료 타이머 가동
        self.daq_process.start(os.path.join(CURRENT_DIR, "frontend"), ["-f", self.combo_config.currentData(), "-o", out_root])
        
        msec = self.spin_chunk_min.value() * 60 * 1000
        self.subrun_timer.start(msec)

    def trigger_subrun_rotation(self):
        """타이머가 완료되면 현재 DAQ에 종료 신호를 보냄 -> process_finished에서 다음 chunk 시작"""
        self.subrun_timer.stop()
        self.print_log(f"[AUTO] Chunk time reached. Rotating to next file...", is_error=True)
        self.daq_process.terminate()

    # ==========================================================
    # 5. 프로세스 통신 및 종료 제어
    # ==========================================================
    def force_stop_daq(self):
        """사용자가 강제로 멈출 경우 모든 자동화 스케줄 파기"""
        self.auto_mode = "NONE"
        self.scan_queue.clear()
        self.subrun_timer.stop()
        if self.daq_process.state() == QProcess.Running:
            self.print_log("[SYSTEM] User Intervened: Force stopping DAQ gracefully...", is_error=True)
            self.daq_process.terminate()

    def auto_finish(self):
        self.auto_mode = "NONE"
        self.lbl_dash_mode.setText("IDLE")
        self.btn_stop.setEnabled(False)
        self.tabs.setEnabled(True)
        self.print_log("\n[SYSTEM] === All Automated Sequences Completed Successfully! ===")

    def handle_stdout(self):
        raw_data = self.daq_process.readAllStandardOutput().data().decode("utf8", errors="replace")
        lines = raw_data.replace('\r', '\n').split('\n')
        
        for line in lines:
            if not line.strip(): continue
            match_ev = re.search(r'Events:\s*(\d+)', line)
            if match_ev:
                self.lcd_events.display(int(match_ev.group(1)))
                continue 
            self.print_log(line)

    def handle_stderr(self):
        data = self.daq_process.readAllStandardError().data().decode("utf8", errors="replace")
        self.print_log(data, is_error=True)

    def process_finished(self):
        self.lcd_events.display(0)
        
        # DB 업데이트
        if hasattr(self, 'current_run_id'):
            conn = sqlite3.connect(DB_NAME)
            cursor = conn.cursor()
            cursor.execute("UPDATE run_history SET status = 'COMPLETED' WHERE id = ?", (self.current_run_id,))
            conn.commit()
            conn.close()
            self.load_history()

        # 자동화 상태 머신 분기
        if self.auto_mode == "SCAN":
            self.print_log(f"[AUTO] Step THR={self.scan_current_val} finished. Preparing next...")
            # 다음 큐가 바로 실행되면 충돌날 수 있으니 1초 딜레이 후 실행
            QTimer.singleShot(1000, self.run_next_scan_step)
            
        elif self.auto_mode == "SUBRUN":
            self.current_subrun_idx += 1
            self.print_log(f"[AUTO] Chunk finished. Preparing next chunk...")
            QTimer.singleShot(1000, self.run_next_subrun)
            
        elif self.auto_mode == "NONE":
            self.auto_finish()

    def closeEvent(self, event):
        if self.daq_process.state() == QProcess.Running or self.monitor_process.state() == QProcess.Running:
            reply = QMessageBox.question(self, 'System Alert',
                "DAQ or Monitor is running!\nStop all processes and exit?",
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.Yes:
                self.force_stop_daq()
                if self.daq_process.state() == QProcess.Running:
                    self.daq_process.waitForFinished(3000)
                if self.monitor_process.state() == QProcess.Running:
                    self.monitor_process.terminate()
                event.accept()
            else:
                event.ignore()
        else:
            event.accept()

if __name__ == "__main__":
    init_db()
    app = QApplication(sys.argv)
    window = DAQControlCenter()
    window.show()
    sys.exit(app.exec_())
