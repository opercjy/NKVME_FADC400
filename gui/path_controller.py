#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
from PyQt5.QtWidgets import QWidget, QHBoxLayout, QLabel, QLineEdit, QPushButton, QFileDialog
from PyQt5.QtCore import pyqtSignal

class PathControllerWidget(QWidget):
    """
    데이터 출력 및 저장 경로를 관리하는 독립적인 재사용 가능 모듈 (OOP)
    """
    sig_path_changed = pyqtSignal(str)

    def __init__(self, title="Save Path:", default_path="", parent=None):
        super().__init__(parent)
        self.current_path = default_path
        self.init_ui(title)

    def init_ui(self, title):
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        
        lbl = QLabel(title)
        lbl.setStyleSheet("font-weight: bold; color: #1565C0;")
        
        self.input_path = QLineEdit(self.current_path)
        self.input_path.setReadOnly(True)
        self.input_path.setStyleSheet("background-color: #F5F5F5; color: #424242;")
        
        btn_browse = QPushButton("📂 경로 변경...")
        btn_browse.setStyleSheet("background-color: #5C6BC0; color: white; font-weight: bold;")
        btn_browse.clicked.connect(self.browse_path)
        
        layout.addWidget(lbl)
        layout.addWidget(self.input_path, stretch=1)
        layout.addWidget(btn_browse)

    def browse_path(self):
        new_dir = QFileDialog.getExistingDirectory(self, "Select Directory", self.current_path)
        if new_dir:
            self.current_path = new_dir
            self.input_path.setText(self.current_path)
            self.sig_path_changed.emit(self.current_path)
            
    def get_path(self):
        return self.current_path