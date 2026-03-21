#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QPushButton, 
                             QLabel, QGroupBox, QComboBox, QTextEdit, 
                             QFileDialog, QLineEdit, QMessageBox)
from PyQt5.QtCore import pyqtSignal
from PyQt5.QtGui import QFont

class ConfigManagerWidget(QWidget):
    # 설정이 변경되거나 새 파일이 저장되면 메인 GUI에 알림을 보내는 시그널
    sig_config_updated = pyqtSignal()
    sig_dir_changed = pyqtSignal(str)

    def __init__(self, config_dir, default_data_dir, parent=None):
        super().__init__(parent)
        self.config_dir = config_dir
        self.current_data_dir = default_data_dir
        
        # config 폴더가 없으면 생성
        if not os.path.exists(self.config_dir):
            os.makedirs(self.config_dir)

        self.init_ui()
        self.refresh_file_list()

    def init_ui(self):
        main_layout = QVBoxLayout(self)

        # --- 1. 데이터 저장 경로 설정 ---
        path_group = QGroupBox("1. Data Storage Path (.root 파일 저장 위치)")
        path_group.setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #9C27B0; border-radius: 8px; margin-top: 15px; } QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 15px; color: #9C27B0; }")
        path_layout = QHBoxLayout()
        path_layout.setContentsMargins(15, 25, 15, 15)
        
        self.input_dir = QLineEdit(self.current_data_dir)
        self.input_dir.setReadOnly(True)
        self.input_dir.setStyleSheet("background-color: #F5F5F5; color: #333; padding: 5px;")
        
        btn_dir = QPushButton("📂 Change Directory")
        btn_dir.setStyleSheet("background-color: #9C27B0; color: white; font-weight: bold; padding: 5px 15px;")
        btn_dir.clicked.connect(self.select_directory)

        path_layout.addWidget(QLabel("Output Dir:"))
        path_layout.addWidget(self.input_dir, stretch=1)
        path_layout.addWidget(btn_dir)
        path_group.setLayout(path_layout)
        main_layout.addWidget(path_group)

        # --- 2. 설정 파일 에디터 ---
        edit_group = QGroupBox("2. Configuration File Editor (*.config)")
        edit_group.setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #009688; border-radius: 8px; margin-top: 15px; } QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 15px; color: #009688; }")
        edit_layout = QVBoxLayout()
        edit_layout.setContentsMargins(15, 25, 15, 15)

        # 파일 선택 콤보박스
        top_edit_layout = QHBoxLayout()
        self.combo_files = QComboBox()
        self.combo_files.currentIndexChanged.connect(self.load_file_content)
        
        btn_refresh = QPushButton("🔄 Refresh")
        btn_refresh.clicked.connect(self.refresh_file_list)
        
        top_edit_layout.addWidget(QLabel("Target Config:"))
        top_edit_layout.addWidget(self.combo_files, stretch=1)
        top_edit_layout.addWidget(btn_refresh)
        edit_layout.addLayout(top_edit_layout)

        # 텍스트 에디터
        self.text_editor = QTextEdit()
        self.text_editor.setFont(QFont("Consolas", 12))
        self.text_editor.setStyleSheet("background-color: #FAFAFA; border: 1px solid #CCC; padding: 10px;")
        edit_layout.addWidget(self.text_editor)

        # 저장 버튼들
        btn_layout = QHBoxLayout()
        btn_save = QPushButton("💾 Save (덮어쓰기)")
        btn_save.setStyleSheet("background-color: #4CAF50; color: white; padding: 8px; font-weight: bold;")
        btn_save.clicked.connect(self.save_file)
        
        btn_save_as = QPushButton("📝 Save As... (다른 이름으로 저장)")
        btn_save_as.setStyleSheet("background-color: #2196F3; color: white; padding: 8px; font-weight: bold;")
        btn_save_as.clicked.connect(self.save_as_file)
        
        btn_layout.addStretch()
        btn_layout.addWidget(btn_save)
        btn_layout.addWidget(btn_save_as)
        edit_layout.addLayout(btn_layout)

        edit_group.setLayout(edit_layout)
        main_layout.addWidget(edit_group, stretch=1)

    def select_directory(self):
        dir_path = QFileDialog.getExistingDirectory(self, "Select Data Storage Directory", self.current_data_dir)
        if dir_path:
            self.current_data_dir = dir_path
            self.input_dir.setText(dir_path)
            self.sig_dir_changed.emit(dir_path)
            QMessageBox.information(self, "Success", f"데이터 저장 경로가 변경되었습니다:\n{dir_path}")

    def refresh_file_list(self):
        self.combo_files.blockSignals(True)
        self.combo_files.clear()
        files = [f for f in os.listdir(self.config_dir) if f.endswith('.config')]
        for f in sorted(files):
            self.combo_files.addItem(f, os.path.join(self.config_dir, f))
        self.combo_files.blockSignals(False)
        
        if self.combo_files.count() > 0:
            self.load_file_content()
        else:
            self.text_editor.clear()

    def load_file_content(self):
        filepath = self.combo_files.currentData()
        if filepath and os.path.exists(filepath):
            with open(filepath, 'r') as f:
                self.text_editor.setText(f.read())

    def save_file(self):
        filepath = self.combo_files.currentData()
        if not filepath:
            QMessageBox.warning(self, "Warning", "선택된 파일이 없습니다.")
            return
            
        with open(filepath, 'w') as f:
            f.write(self.text_editor.toPlainText())
        QMessageBox.information(self, "Success", "파일이 성공적으로 저장되었습니다.")
        self.sig_config_updated.emit() # 메인 GUI 업데이트 트리거

    def save_as_file(self):
        filename, _ = QFileDialog.getSaveFileName(self, "Save Config As", self.config_dir, "Config Files (*.config)")
        if filename:
            if not filename.endswith(".config"):
                filename += ".config"
            with open(filename, 'w') as f:
                f.write(self.text_editor.toPlainText())
            QMessageBox.information(self, "Success", f"새 파일로 저장되었습니다:\n{os.path.basename(filename)}")
            self.refresh_file_list()
            
            # 콤보박스 선택을 방금 저장한 파일로 이동
            idx = self.combo_files.findText(os.path.basename(filename))
            if idx >= 0: self.combo_files.setCurrentIndex(idx)
            
            self.sig_config_updated.emit()

    def get_current_data_dir(self):
        return self.current_data_dir