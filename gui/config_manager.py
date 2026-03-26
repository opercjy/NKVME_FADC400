#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import glob
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QPushButton, 
                             QLabel, QGroupBox, QComboBox, QTextEdit, QMessageBox)
from PyQt5.QtCore import pyqtSignal
from PyQt5.QtGui import QFont

# [핵심] 재사용 가능한 경로 컨트롤러 객체 임포트
from path_controller import PathControllerWidget

class ConfigManagerWidget(QWidget):
    sig_config_updated = pyqtSignal()
    sig_config_dir_changed = pyqtSignal(str) # Config 경로 변경 시그널

    def __init__(self, config_dir, parent=None):
        super().__init__(parent)
        self.config_dir = config_dir
        
        if not os.path.exists(self.config_dir):
            os.makedirs(self.config_dir)

        self.init_ui()
        self.refresh_file_list()

    def init_ui(self):
        main_layout = QVBoxLayout(self)

        # --- 1. 설정 파일 폴더 경로 (OOP 모듈 부착) ---
        self.path_config = PathControllerWidget("📝 Config Files Directory (*.config):", self.config_dir)
        self.path_config.sig_path_changed.connect(self.on_config_dir_changed)
        main_layout.addWidget(self.path_config)

        # --- 2. 설정 파일 에디터 ---
        edit_group = QGroupBox("Configuration File Editor")
        edit_group.setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #009688; border-radius: 8px; margin-top: 15px; } QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 15px; color: #009688; }")
        edit_layout = QVBoxLayout()
        edit_layout.setContentsMargins(15, 25, 15, 15)

        top_edit_layout = QHBoxLayout()
        self.combo_files = QComboBox()
        self.combo_files.currentIndexChanged.connect(self.load_file_content)
        
        btn_refresh = QPushButton("🔄 Refresh")
        btn_refresh.clicked.connect(self.refresh_file_list)
        
        top_edit_layout.addWidget(QLabel("Target Config:"))
        top_edit_layout.addWidget(self.combo_files, stretch=1)
        top_edit_layout.addWidget(btn_refresh)
        edit_layout.addLayout(top_edit_layout)

        self.text_editor = QTextEdit()
        self.text_editor.setFont(QFont("Consolas", 12))
        self.text_editor.setStyleSheet("background-color: #FAFAFA; border: 1px solid #CCC; padding: 10px;")
        edit_layout.addWidget(self.text_editor)

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

    def on_config_dir_changed(self, new_dir):
        self.config_dir = new_dir
        self.refresh_file_list()
        self.sig_config_dir_changed.emit(new_dir)

    def refresh_file_list(self):
        self.combo_files.blockSignals(True)
        self.combo_files.clear()
        if os.path.exists(self.config_dir):
            files = [f for f in os.listdir(self.config_dir) if f.endswith('.config')]
            for f in sorted(files):
                if f != "temp_auto.config": # 임시 스캔 파일은 에디터에서 숨김
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
        self.sig_config_updated.emit()

    def save_as_file(self):
        from PyQt5.QtWidgets import QFileDialog
        filename, _ = QFileDialog.getSaveFileName(self, "Save Config As", self.config_dir, "Config Files (*.config)")
        if filename:
            if not filename.endswith(".config"): filename += ".config"
            with open(filename, 'w') as f:
                f.write(self.text_editor.toPlainText())
            QMessageBox.information(self, "Success", f"새 파일로 저장되었습니다:\n{os.path.basename(filename)}")
            self.refresh_file_list()
            idx = self.combo_files.findText(os.path.basename(filename))
            if idx >= 0: self.combo_files.setCurrentIndex(idx)
            self.sig_config_updated.emit()