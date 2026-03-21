#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sqlite3
from datetime import datetime
from PyQt5.QtWidgets import (QVBoxLayout, QPushButton, QTableWidget, 
                             QTableWidgetItem, QGroupBox, QHeaderView)
from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QColor

class ELogbookWidget(QGroupBox):
    # 메인 윈도우의 콘솔에 로그를 전달하기 위한 시그널
    sig_log = pyqtSignal(str, bool)

    def __init__(self, db_path, parent=None):
        super().__init__("Run History (E-Logbook)", parent)
        self.db_path = db_path
        self.init_db()
        self.init_ui()
        self.load_history()

    def init_db(self):
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        cur.execute('''CREATE TABLE IF NOT EXISTS run_history 
                       (id INTEGER PRIMARY KEY AUTOINCREMENT, run_num TEXT, run_type TEXT, 
                        description TEXT, start_time TEXT, status TEXT)''')
        conn.commit()
        conn.close()

    def init_ui(self):
        layout = QVBoxLayout()
        layout.setContentsMargins(5, 5, 5, 5)
        
        self.table = QTableWidget(0, 6)
        self.table.setHorizontalHeaderLabels(["ID", "Run", "Type", "Description", "Start Time", "Status"])
        self.table.horizontalHeader().setSectionResizeMode(3, QHeaderView.Stretch)
        self.table.horizontalHeader().setSectionResizeMode(5, QHeaderView.ResizeToContents)
        layout.addWidget(self.table)
        
        self.btn_save_db = QPushButton("💾 Save Modified History")
        self.btn_save_db.setStyleSheet("background-color: #4CAF50; color: white;")
        self.btn_save_db.clicked.connect(self.save_history)
        layout.addWidget(self.btn_save_db)
        
        self.setLayout(layout)

    def load_history(self):
        self.table.setRowCount(0)
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        cur.execute("SELECT * FROM run_history ORDER BY id DESC LIMIT 50")
        for row_idx, row_data in enumerate(cur.fetchall()):
            self.table.insertRow(row_idx)
            for col_idx, col_data in enumerate(row_data):
                item = QTableWidgetItem(str(col_data))
                if col_idx == 0: 
                    item.setFlags(item.flags() ^ Qt.ItemIsEditable) 
                elif col_idx == 5: 
                    item.setFlags(item.flags() ^ Qt.ItemIsEditable)
                    if "RUNNING" in str(col_data): item.setForeground(QColor("red"))
                    elif "COMPLETED" in str(col_data): item.setForeground(QColor("green"))
                self.table.setItem(row_idx, col_idx, item)
        conn.close()

    def save_history(self):
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        for row in range(self.table.rowCount()):
            idx = self.table.item(row, 0).text()
            run_num = self.table.item(row, 1).text()
            run_type = self.table.item(row, 2).text()
            desc = self.table.item(row, 3).text()
            cur.execute("UPDATE run_history SET run_num=?, run_type=?, description=? WHERE id=?", 
                        (run_num, run_type, desc, idx))
        conn.commit()
        conn.close()
        self.sig_log.emit("\033[1;32m[DB]\033[0m E-Logbook modifications saved.", False)
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
        return str(max_num + 1) if max_num > 0 else "100"

    def record_run(self, run_num, run_type, desc):
        start_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        cur.execute("INSERT INTO run_history (run_num, run_type, description, start_time, status) VALUES (?, ?, ?, ?, ?)",
                    (run_num, run_type, desc, start_time, "RUNNING"))
        run_id = cur.lastrowid
        conn.commit()
        conn.close()
        self.load_history()
        return run_id

    def update_status(self, run_id, status_str):
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        cur.execute("UPDATE run_history SET status = ? WHERE id = ?", (status_str, run_id))
        conn.commit()
        conn.close()
        self.load_history()