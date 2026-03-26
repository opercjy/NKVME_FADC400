#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sqlite3
import re
from datetime import datetime
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QPushButton, 
                             QTableWidget, QTableWidgetItem, QHeaderView, QLabel, QMessageBox, QTabWidget)
from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QColor, QFont

class ELogbookWidget(QWidget):
    sig_log = pyqtSignal(str, bool)

    def __init__(self, db_path, parent=None):
        super().__init__(parent)
        self.db_path = db_path
        self.init_db()
        self.init_ui()
        self.load_history()

    def init_db(self):
        os.makedirs(os.path.dirname(self.db_path), exist_ok=True)
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        
        # 💡 [버그 픽스] 원래의 컬럼명(timestamp, mode, tag)으로 완벽히 복원
        cur.execute('''
            CREATE TABLE IF NOT EXISTS run_history (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp TEXT,
                run_num TEXT,
                mode TEXT,
                tag TEXT,
                quality TEXT,
                status TEXT,
                description TEXT
            )
        ''')
        
        cur.execute("PRAGMA table_info(run_history)")
        cols = [info[1] for info in cur.fetchall()]
        
        # 이전 버전 사용자를 위한 자동 마이그레이션 (안전 장치)
        if 'tag' not in cols: cur.execute("ALTER TABLE run_history ADD COLUMN tag TEXT DEFAULT 'TEST'")
        if 'quality' not in cols: cur.execute("ALTER TABLE run_history ADD COLUMN quality TEXT DEFAULT 'PENDING'")
        if 'hv_voltage' not in cols: cur.execute("ALTER TABLE run_history ADD COLUMN hv_voltage TEXT DEFAULT ''")
            
        # 오프라인 분석(Production) DB 생성
        cur.execute('''CREATE TABLE IF NOT EXISTS prod_history (
                id INTEGER PRIMARY KEY AUTOINCREMENT, timestamp TEXT, raw_file TEXT, prod_file TEXT,
                mode TEXT, events INTEGER, elapsed_sec REAL, speed_hz REAL)''')
                
        conn.commit()
        conn.close()

    def init_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(15, 15, 15, 15)
        
        lbl_title = QLabel("🗄️ Run Database & E-Logbook Explorer")
        lbl_title.setFont(QFont("Arial", 14, QFont.Bold))
        lbl_title.setStyleSheet("color: #4CAF50;")
        layout.addWidget(lbl_title)
        
        self.tabs = QTabWidget()
        
        self.run_table = QTableWidget(0, 9)
        self.run_table.setHorizontalHeaderLabels(["ID", "Time", "Run", "Mode", "Tag", "HV(V)", "Quality", "Status", "Description"])
        self.run_table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)
        self.run_table.horizontalHeader().setSectionResizeMode(8, QHeaderView.Stretch)
        self.run_table.setWordWrap(True)
        self.run_table.verticalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)
        self.run_table.setStyleSheet("QTableWidget { font-size: 13px; } QHeaderView::section { font-weight: bold; background-color: #E0E0E0; }")
        
        self.prod_table = QTableWidget(0, 8)
        self.prod_table.setHorizontalHeaderLabels(["ID", "Time", "Raw File", "Prod File", "Mode", "Events", "Time(s)", "Speed(Hz)"])
        self.prod_table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)
        self.prod_table.horizontalHeader().setSectionResizeMode(3, QHeaderView.Stretch)
        
        self.tabs.addTab(self.run_table, "📡 DAQ Run History")
        self.tabs.addTab(self.prod_table, "📦 Offline Production History")
        
        layout.addWidget(self.tabs)

        btn_layout = QHBoxLayout()
        self.btn_refresh = QPushButton("🔄 Refresh DB")
        self.btn_refresh.clicked.connect(self.load_history)
        
        btn_export = QPushButton("📤 Export to CSV")
        btn_export.clicked.connect(self.export_csv)
        
        self.btn_save_db = QPushButton("💾 Save Modified Information")
        self.btn_save_db.setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold; padding: 10px;")
        self.btn_save_db.clicked.connect(self.save_history)
        
        btn_layout.addWidget(self.btn_refresh)
        btn_layout.addWidget(btn_export)
        btn_layout.addStretch()
        btn_layout.addWidget(self.btn_save_db)
        layout.addLayout(btn_layout)

    def load_history(self):
        self.run_table.setRowCount(0)
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        
        try:
            # 💡 [버그 픽스] timestamp, mode 로 다시 호출
            cur.execute("SELECT id, timestamp, run_num, mode, tag, hv_voltage, quality, status, description FROM run_history ORDER BY id DESC LIMIT 100")
            for row_idx, row_data in enumerate(cur.fetchall()):
                self.run_table.insertRow(row_idx)
                for col_idx, col_data in enumerate(row_data):
                    item = QTableWidgetItem(str(col_data) if col_data else "")
                    if col_idx in [0, 1, 2, 7]: # 고정 컬럼 읽기 전용
                        item.setFlags(item.flags() ^ Qt.ItemIsEditable) 
                        item.setBackground(QColor("#F5F5F5"))
                    
                    # 상태 색상 처리
                    if col_idx == 7: 
                        if "RUNNING" in str(col_data): item.setForeground(QColor("red"))
                        elif "COMPLETED" in str(col_data): item.setForeground(QColor("green"))
                    elif col_idx == 6: 
                        if "GOOD" in str(col_data).upper(): item.setForeground(QColor("blue"))
                        elif "BAD" in str(col_data).upper(): item.setForeground(QColor("red"))
                    elif col_idx == 4: item.setForeground(QColor("#9C27B0"))
                    elif col_idx == 5: item.setForeground(QColor("#E65100"))

                    self.run_table.setItem(row_idx, col_idx, item)
        except Exception as e:
            self.sig_log.emit(f"[DB] Load Error: {e}", True)
                
        self.prod_table.setRowCount(0)
        try:
            cur.execute("SELECT * FROM prod_history ORDER BY id DESC")
            for row_idx, row_data in enumerate(cur.fetchall()):
                self.prod_table.insertRow(row_idx)
                for col_idx, col_data in enumerate(row_data):
                    self.prod_table.setItem(row_idx, col_idx, QTableWidgetItem(str(col_data)))
        except: pass
        conn.close()

    def save_history(self):
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        for row in range(self.run_table.rowCount()):
            idx = self.run_table.item(row, 0).text()
            run_type = self.run_table.item(row, 3).text()
            run_tag = self.run_table.item(row, 4).text()
            hv_volt = self.run_table.item(row, 5).text()
            quality = self.run_table.item(row, 6).text()
            desc = self.run_table.item(row, 8).text()
            
            cur.execute("UPDATE run_history SET mode=?, tag=?, hv_voltage=?, quality=?, description=? WHERE id=?", 
                        (run_type, run_tag, hv_volt, quality, desc, idx))
        conn.commit()
        conn.close()
        self.sig_log.emit("\033[1;32m[DB]\033[0m Database modifications saved.", False)
        self.load_history()

    def get_next_run_number(self):
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        cur.execute("SELECT run_num FROM run_history ORDER BY id DESC LIMIT 10")
        rows = cur.fetchall()
        conn.close()
        max_num = 0
        for row in rows:
            run_str = str(row[0]).split('_')[0]
            if run_str.isdigit() and int(run_str) > max_num:
                max_num = int(run_str)
        return str(max_num + 1) if max_num > 0 else "101"

    def record_run(self, run_num, run_type, run_tag, hv_volt, quality, desc):
        start_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        # 💡 [버그 픽스] timestamp, mode 로 정확히 매핑하여 삽입
        cur.execute("INSERT INTO run_history (run_num, mode, tag, hv_voltage, quality, description, timestamp, status) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                    (run_num, run_type, run_tag, hv_volt, quality, desc, start_time, "RUNNING"))
        run_id = cur.lastrowid
        conn.commit()
        conn.close()
        self.load_history()
        self.sig_log.emit(f"\033[1;32m[DB]\033[0m Run {run_num} registered (ID: {run_id}).", False)
        return run_id

    def update_status(self, run_id, status_str):
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        cur.execute("UPDATE run_history SET status = ? WHERE id = ?", (status_str, run_id))
        conn.commit()
        conn.close()
        self.load_history()
        
    def record_production(self, raw_file, prod_file, mode, events, elapsed, speed):
        ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        cur.execute('''INSERT INTO prod_history (timestamp, raw_file, prod_file, mode, events, elapsed_sec, speed_hz)
                       VALUES (?, ?, ?, ?, ?, ?, ?)''', (ts, raw_file, prod_file, mode, events, elapsed, speed))
        conn.commit()
        conn.close()
        self.load_history()
        self.sig_log.emit(f"\033[1;32m[DB]\033[0m Production log saved: {prod_file}", False)

    def export_csv(self):
        import csv
        out_run = self.db_path.replace(".db", "_run.csv")
        out_prod = self.db_path.replace(".db", "_prod.csv")
        try:
            with open(out_run, 'w', newline='') as f:
                writer = csv.writer(f)
                writer.writerow(["ID", "Time", "Run", "Mode", "Tag", "HV(V)", "Quality", "Status", "Description"])
                conn = sqlite3.connect(self.db_path)
                cur = conn.cursor()
                cur.execute("SELECT id, timestamp, run_num, mode, tag, hv_voltage, quality, status, description FROM run_history ORDER BY id DESC")
                writer.writerows(cur.fetchall())
            with open(out_prod, 'w', newline='') as f:
                writer = csv.writer(f)
                writer.writerow(["ID", "Time", "Raw File", "Prod File", "Mode", "Events", "Time(s)", "Speed(Hz)"])
                cur.execute("SELECT * FROM prod_history ORDER BY id DESC")
                writer.writerows(cur.fetchall())
                conn.close()
            QMessageBox.information(self, "Export", f"CSV Exported successfully to:\n{os.path.dirname(out_run)}")
        except Exception as e:
            QMessageBox.warning(self, "Export Error", str(e))