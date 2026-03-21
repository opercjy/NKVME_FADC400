#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sqlite3
from datetime import datetime
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QPushButton, 
                             QTableWidget, QTableWidgetItem, QHeaderView, QLabel, QComboBox)
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
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        # 기본 테이블 생성
        cur.execute('''CREATE TABLE IF NOT EXISTS run_history 
                       (id INTEGER PRIMARY KEY AUTOINCREMENT, run_num TEXT, run_type TEXT, 
                        description TEXT, start_time TEXT, status TEXT)''')
        
        # [핵심] 기존 DB에 Tag, Quality 컬럼이 없다면 자동 추가 (마이그레이션)
        cur.execute("PRAGMA table_info(run_history)")
        cols = [info[1] for info in cur.fetchall()]
        if 'run_tag' not in cols:
            cur.execute("ALTER TABLE run_history ADD COLUMN run_tag TEXT DEFAULT 'TEST'")
        if 'quality' not in cols:
            cur.execute("ALTER TABLE run_history ADD COLUMN quality TEXT DEFAULT 'PENDING'")
            
        conn.commit()
        conn.close()

    def init_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(15, 15, 15, 15)
        
        lbl_title = QLabel("🗄️ Run Database & E-Logbook Explorer")
        lbl_title.setFont(QFont("Arial", 14, QFont.Bold))
        lbl_title.setStyleSheet("color: #4CAF50;")
        layout.addWidget(lbl_title)
        
        # 8개 컬럼으로 확장
        self.table = QTableWidget(0, 8)
        self.table.setHorizontalHeaderLabels(["ID", "Run", "Type", "Tag (분류)", "Quality (품질)", "Description (코멘트)", "Start Time", "Status"])
        
        # [핵심] 긴 코멘트에 대비한 레이아웃 설정
        self.table.horizontalHeader().setSectionResizeMode(5, QHeaderView.Stretch) # Description이 남은 공간 다 차지
        self.table.horizontalHeader().setSectionResizeMode(7, QHeaderView.ResizeToContents) # Status 크기 맞춤
        self.table.setWordWrap(True) # 텍스트 자동 줄바꿈
        self.table.verticalHeader().setSectionResizeMode(QHeaderView.ResizeToContents) # 행 높이 자동 조절
        
        self.table.setStyleSheet("QTableWidget { font-size: 13px; } QHeaderView::section { font-weight: bold; background-color: #E0E0E0; }")
        layout.addWidget(self.table)
        
        btn_layout = QHBoxLayout()
        self.btn_refresh = QPushButton("🔄 Refresh DB")
        self.btn_refresh.clicked.connect(self.load_history)
        
        self.btn_save_db = QPushButton("💾 Save Modified Quality & Descriptions")
        self.btn_save_db.setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold; padding: 10px;")
        self.btn_save_db.clicked.connect(self.save_history)
        
        btn_layout.addWidget(self.btn_refresh)
        btn_layout.addStretch()
        btn_layout.addWidget(self.btn_save_db)
        layout.addLayout(btn_layout)

    def load_history(self):
        self.table.setRowCount(0)
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        cur.execute("SELECT id, run_num, run_type, run_tag, quality, description, start_time, status FROM run_history ORDER BY id DESC LIMIT 100")
        for row_idx, row_data in enumerate(cur.fetchall()):
            self.table.insertRow(row_idx)
            for col_idx, col_data in enumerate(row_data):
                item = QTableWidgetItem(str(col_data) if col_data else "")
                
                # 수정 불가 컬럼들
                if col_idx in [0, 1, 6, 7]: 
                    item.setFlags(item.flags() ^ Qt.ItemIsEditable) 
                    item.setBackground(QColor("#F5F5F5"))
                
                # 색상 하이라이트
                if col_idx == 7: # Status
                    if "RUNNING" in str(col_data): item.setForeground(QColor("red"))
                    elif "COMPLETED" in str(col_data): item.setForeground(QColor("green"))
                elif col_idx == 4: # Quality
                    if "GOOD" in str(col_data).upper(): item.setForeground(QColor("blue"))
                    elif "BAD" in str(col_data).upper(): item.setForeground(QColor("red"))
                elif col_idx == 3: # Tag
                    item.setForeground(QColor("#9C27B0"))

                self.table.setItem(row_idx, col_idx, item)
        conn.close()

    def save_history(self):
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        for row in range(self.table.rowCount()):
            idx = self.table.item(row, 0).text()
            run_type = self.table.item(row, 2).text()
            run_tag = self.table.item(row, 3).text()
            quality = self.table.item(row, 4).text()
            desc = self.table.item(row, 5).text()
            
            cur.execute("UPDATE run_history SET run_type=?, run_tag=?, quality=?, description=? WHERE id=?", 
                        (run_type, run_tag, quality, desc, idx))
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
        return str(max_num + 1) if max_num > 0 else "100"

    def record_run(self, run_num, run_type, run_tag, quality, desc):
        start_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        conn = sqlite3.connect(self.db_path)
        cur = conn.cursor()
        cur.execute("INSERT INTO run_history (run_num, run_type, run_tag, quality, description, start_time, status) VALUES (?, ?, ?, ?, ?, ?, ?)",
                    (run_num, run_type, run_tag, quality, desc, start_time, "RUNNING"))
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