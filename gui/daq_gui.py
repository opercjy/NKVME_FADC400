#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import sqlite3
from datetime import datetime
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                             QHBoxLayout, QPushButton, QLabel, QLineEdit, 
                             QTableWidget, QTableWidgetItem, QGroupBox, QHeaderView, QMessageBox)
from PyQt5.QtCore import QProcess, Qt

# [핵심] 현재 스크립트 실행 위치(bin/)를 기반으로 프로젝트 루트 경로 추적
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(CURRENT_DIR) 

DB_NAME = os.path.join(PROJECT_ROOT, "daq_history.db")
CONFIG_PATH = os.path.join(PROJECT_ROOT, "config", "ch1.config")

def init_db():
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS run_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            run_num INTEGER,
            run_type TEXT,
            description TEXT,
            start_time TEXT,
            status TEXT
        )
    ''')
    conn.commit()
    conn.close()

class DAQControlCenter(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("NoticeDAQ Control Center")
        self.resize(850, 600)
        
        self.daq_process = QProcess(self)
        self.daq_process.readyReadStandardOutput.connect(self.handle_stdout)
        self.daq_process.finished.connect(self.process_finished)

        self.init_ui()
        self.load_history()

    def init_ui(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)

        # 1. Run Configuration
        config_group = QGroupBox("Run Configuration")
        config_layout = QHBoxLayout()
        
        self.input_runnum = QLineEdit()
        self.input_runnum.setPlaceholderText("e.g. 101")
        self.input_runtype = QLineEdit()
        self.input_runtype.setPlaceholderText("e.g. TEST")
        self.input_desc = QLineEdit()
        self.input_desc.setPlaceholderText("Run Description...")

        config_layout.addWidget(QLabel("Run Num:"))
        config_layout.addWidget(self.input_runnum)
        config_layout.addWidget(QLabel("Type:"))
        config_layout.addWidget(self.input_runtype)
        config_layout.addWidget(QLabel("Desc:"))
        config_layout.addWidget(self.input_desc, stretch=1)
        
        config_group.setLayout(config_layout)
        main_layout.addWidget(config_group)

        # 2. System Controls
        control_group = QGroupBox("System Controls")
        control_layout = QHBoxLayout()

        self.btn_monitor = QPushButton("📊 Launch Online Monitor")
        self.btn_monitor.setStyleSheet("background-color: #9C27B0; color: white; font-weight: bold; padding: 10px;")
        self.btn_monitor.clicked.connect(self.launch_monitor)

        self.btn_start = QPushButton("▶ Start DAQ")
        self.btn_start.setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold; padding: 10px;")
        self.btn_start.clicked.connect(self.start_daq)

        self.btn_stop = QPushButton("■ Stop DAQ")
        self.btn_stop.setStyleSheet("background-color: #F44336; color: white; font-weight: bold; padding: 10px;")
        self.btn_stop.setEnabled(False)
        self.btn_stop.clicked.connect(self.stop_daq)

        self.btn_prod = QPushButton("⚙ Run Offline Analysis")
        self.btn_prod.setStyleSheet("background-color: #2196F3; color: white; font-weight: bold; padding: 10px;")
        self.btn_prod.clicked.connect(self.run_production)

        control_layout.addWidget(self.btn_monitor)
        control_layout.addWidget(self.btn_start)
        control_layout.addWidget(self.btn_stop)
        control_layout.addWidget(self.btn_prod)
        
        control_group.setLayout(control_layout)
        main_layout.addWidget(control_group)

        # 3. Database Table
        db_group = QGroupBox("Run History (E-Logbook)")
        db_layout = QVBoxLayout()
        
        self.table = QTableWidget(0, 6)
        self.table.setHorizontalHeaderLabels(["ID", "Run Num", "Type", "Description", "Start Time", "Status"])
        self.table.horizontalHeader().setSectionResizeMode(3, QHeaderView.Stretch)
        
        db_layout.addWidget(self.table)
        db_group.setLayout(db_layout)
        main_layout.addWidget(db_group)

    def load_history(self):
        self.table.setRowCount(0)
        conn = sqlite3.connect(DB_NAME)
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM run_history ORDER BY id DESC LIMIT 50")
        for row_idx, row_data in enumerate(cursor.fetchall()):
            self.table.insertRow(row_idx)
            for col_idx, col_data in enumerate(row_data):
                self.table.setItem(row_idx, col_idx, QTableWidgetItem(str(col_data)))
        conn.close()

    def launch_monitor(self):
        monitor_exe = os.path.join(CURRENT_DIR, "OnlineMonitor")
        QProcess.startDetached(monitor_exe, [])
        self.statusBar().showMessage("Online Monitor launched in background.")

    def start_daq(self):
        run_num = self.input_runnum.text().strip()
        run_type = self.input_runtype.text().strip()
        desc = self.input_desc.text().strip()

        if not run_num:
            QMessageBox.warning(self, "Warning", "Run Number를 입력하세요!")
            return

        start_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        conn = sqlite3.connect(DB_NAME)
        cursor = conn.cursor()
        cursor.execute("INSERT INTO run_history (run_num, run_type, description, start_time, status) VALUES (?, ?, ?, ?, ?)",
                       (run_num, run_type, desc, start_time, "RUNNING"))
        self.current_run_id = cursor.lastrowid
        conn.commit()
        conn.close()
        
        self.load_history()

        out_root = os.path.join(PROJECT_ROOT, f"run_{run_num}.root")
        frontend_exe = os.path.join(CURRENT_DIR, "frontend") 
        
        self.daq_process.start(frontend_exe, ["-f", CONFIG_PATH, "-o", out_root])
        
        self.btn_start.setEnabled(False)
        self.btn_stop.setEnabled(True)
        self.statusBar().showMessage(f"DAQ Started: Run {run_num}")

    def stop_daq(self):
        if self.daq_process.state() == QProcess.Running:
            self.daq_process.terminate() 
            self.statusBar().showMessage("Sending Graceful Shutdown signal to DAQ...")

    def handle_stdout(self):
        data = self.daq_process.readAllStandardOutput().data().decode("utf8")
        print(data, end="") 

    def process_finished(self):
        self.btn_start.setEnabled(True)
        self.btn_stop.setEnabled(False)
        self.statusBar().showMessage("DAQ Stopped Safely.")

        if hasattr(self, 'current_run_id'):
            conn = sqlite3.connect(DB_NAME)
            cursor = conn.cursor()
            cursor.execute("UPDATE run_history SET status = 'COMPLETED' WHERE id = ?", (self.current_run_id,))
            conn.commit()
            conn.close()
            self.load_history()

    def run_production(self):
        run_num = self.input_runnum.text().strip()
        if not run_num:
            QMessageBox.warning(self, "Warning", "분석할 Run Number를 입력하세요!")
            return
            
        in_root = os.path.join(PROJECT_ROOT, f"run_{run_num}.root")
        out_root = os.path.join(PROJECT_ROOT, f"prod_{run_num}.root")
        production_exe = os.path.join(CURRENT_DIR, "production") 
        
        QProcess.startDetached(production_exe, ["-i", in_root, "-o", out_root, "-w"])
        self.statusBar().showMessage(f"Production Analyzer launched for Run {run_num}")

if __name__ == "__main__":
    init_db()
    app = QApplication(sys.argv)
    window = DAQControlCenter()
    window.show()
    sys.exit(app.exec_())
