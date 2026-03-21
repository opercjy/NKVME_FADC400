#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import re
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QPushButton, 
                             QLabel, QGroupBox, QLineEdit, QFileDialog, QProgressBar, 
                             QMessageBox, QCheckBox, QInputDialog)
from PyQt5.QtCore import pyqtSignal, QProcess, Qt

EXE_PRODUCTION = "production_nfadc400"

class ProductionManagerWidget(QWidget):
    sig_log = pyqtSignal(str, bool)

    def __init__(self, current_dir, default_data_dir, parent=None):
        super().__init__(parent)
        self.current_dir = current_dir
        self.data_dir = default_data_dir
        
        self.prod_process = QProcess(self)
        self.prod_process.readyReadStandardOutput.connect(self.handle_stdout)
        self.prod_process.readyReadStandardError.connect(self.handle_stderr)
        self.prod_process.finished.connect(self.process_finished)

        self.init_ui()

    def init_ui(self):
        main_layout = QVBoxLayout(self)
        
        # --- 1. 파일 선택 부 ---
        file_group = QGroupBox("1. Select Raw Data File (.root)")
        file_group.setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #673AB7; border-radius: 8px; margin-top: 15px; } QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 15px; color: #673AB7; }")
        file_layout = QHBoxLayout(); file_layout.setContentsMargins(15, 25, 15, 15)
        
        self.input_prod_file = QLineEdit()
        self.input_prod_file.setReadOnly(True)
        btn_browse = QPushButton("📂 Browse...")
        btn_browse.clicked.connect(self.browse_prod_file)
        
        file_layout.addWidget(self.input_prod_file, stretch=1)
        file_layout.addWidget(btn_browse)
        file_group.setLayout(file_layout)
        main_layout.addWidget(file_group)

        # --- 2. 분석 옵션 및 실행 부 ---
        action_group = QGroupBox("2. Analysis Action & Options")
        action_layout = QVBoxLayout()
        
        self.chk_save_wave = QCheckBox("Save Waveform Time-Series (-w) : 이벤트별 전압 강하 시계열(ns)을 트리에 함께 저장합니다.")
        self.chk_save_wave.setChecked(False)
        self.chk_save_wave.setStyleSheet("color: #E65100; font-weight: bold; padding: 5px;")
        action_layout.addWidget(self.chk_save_wave)
        
        btn_batch = QPushButton("⚙️ Run Batch Production (Create _prod.root & Histogram)")
        btn_batch.setStyleSheet("background-color: #673AB7; color: white; padding: 10px; font-weight:bold; font-size: 13px;")
        btn_batch.clicked.connect(self.run_prod_batch)
        action_layout.addWidget(btn_batch)
        
        btn_inter = QPushButton("📈 Open Interactive Viewer (-d Mode)")
        btn_inter.setStyleSheet("background-color: #009688; color: white; padding: 10px; font-weight:bold; font-size: 13px;")
        btn_inter.clicked.connect(self.run_prod_interactive)
        action_layout.addWidget(btn_inter)
        
        self.prod_progress = QProgressBar()
        self.prod_progress.setAlignment(Qt.AlignCenter)
        self.prod_progress.setFormat("Batch Production Progress: %p%")
        action_layout.addWidget(self.prod_progress)
        
        action_group.setLayout(action_layout)
        main_layout.addWidget(action_group)

        # --- 3. [신규 추가] 인터랙티브 뷰어 전용 컨트롤 버튼 부 ---
        self.interactive_group = QGroupBox("3. Interactive Viewer Controls (-d Mode)")
        self.interactive_group.setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #FF9800; border-radius: 8px; margin-top: 15px; } QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 15px; color: #FF9800; }")
        self.interactive_group.setEnabled(False) # 실행 전에는 비활성화 상태 유지
        
        inter_layout = QHBoxLayout(); inter_layout.setContentsMargins(15, 25, 15, 15)

        btn_prev = QPushButton("⏮ Prev (p)")
        btn_prev.setStyleSheet("background-color: #2196F3; color: white; font-weight: bold; padding: 10px;")
        btn_prev.clicked.connect(lambda: self.send_command('p'))

        btn_next = QPushButton("⏭ Next (n)")
        btn_next.setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold; padding: 10px;")
        btn_next.clicked.connect(lambda: self.send_command('n'))

        btn_jump = QPushButton("🔢 Jump (j)")
        btn_jump.setStyleSheet("background-color: #FFC107; color: black; font-weight: bold; padding: 10px;")
        btn_jump.clicked.connect(self.cmd_jump)

        btn_quit = QPushButton("⏹ Quit (q)")
        btn_quit.setStyleSheet("background-color: #F44336; color: white; font-weight: bold; padding: 10px;")
        btn_quit.clicked.connect(lambda: self.send_command('q'))

        inter_layout.addWidget(btn_prev)
        inter_layout.addWidget(btn_next)
        inter_layout.addWidget(btn_jump)
        inter_layout.addWidget(btn_quit)

        self.interactive_group.setLayout(inter_layout)
        main_layout.addWidget(self.interactive_group)

        main_layout.addStretch()

    def browse_prod_file(self):
        filename, _ = QFileDialog.getOpenFileName(self, "Select Raw ROOT File", self.data_dir, "ROOT Files (*.root)")
        if filename:
            self.input_prod_file.setText(filename)

    def run_prod_batch(self):
        infile = self.input_prod_file.text().strip()
        if not infile or not os.path.exists(infile):
            QMessageBox.warning(self, "Error", "선택된 파일이 없거나 존재하지 않습니다.")
            return
        
        self.interactive_group.setEnabled(False)
        self.prod_progress.setValue(0)
        self.sig_log.emit(f"\033[1;35m[PROD]\033[0m Starting Batch Analysis: {os.path.basename(infile)}", False)
        
        args = ["-i", infile]
        if self.chk_save_wave.isChecked():
            args.append("-w")
            
        self.prod_process.start(os.path.join(self.current_dir, EXE_PRODUCTION), args)

    def run_prod_interactive(self):
        infile = self.input_prod_file.text().strip()
        if not infile or not os.path.exists(infile):
            QMessageBox.warning(self, "Error", "선택된 파일이 없거나 존재하지 않습니다.")
            return
            
        self.interactive_group.setEnabled(True) # [핵심] 뷰어가 실행되면 컨트롤 버튼 활성화
        self.sig_log.emit(f"\033[1;35m[PROD]\033[0m Launching Interactive Viewer (-d): {os.path.basename(infile)}", False)
        self.sig_log.emit(f"\033[1;33m[TIP]\033[0m 아래의 'Interactive Viewer Controls' 패널 버튼을 클릭하여 파형을 넘겨보세요.", False)
        
        self.prod_process.start(os.path.join(self.current_dir, EXE_PRODUCTION), ["-i", infile, "-d"])

    # --- [신규 추가] C++ 코어(STDIN)로 명령어 송신 ---
    def send_command(self, cmd):
        """버튼을 클릭하면 해당 명령문자와 엔터(\n)를 QProcess에 밀어넣습니다."""
        if self.prod_process.state() == QProcess.Running:
            self.prod_process.write((cmd + "\n").encode('utf-8'))

    def cmd_jump(self):
        """Jump 버튼을 누르면 이벤트를 입력받는 팝업을 띄우고 C++로 전송합니다."""
        if self.prod_process.state() != QProcess.Running: return
        num, ok = QInputDialog.getInt(self, "Jump to Event", "이동할 이벤트 번호를 입력하세요:", 0, 0, 2000000000, 1)
        if ok:
            # C++ 코드가 'j'를 받고 나서 숫자를 기다리므로, "j\n123\n" 형태로 한 번에 쏴줍니다.
            self.send_command(f"j\n{num}")
    # --------------------------------------------------

    def handle_stdout(self):
        raw_data = self.prod_process.readAllStandardOutput().data().decode("utf8", errors="replace")
        for line in raw_data.split('\n'):
            for subline in line.split('\r'):
                if not subline.strip(): continue
                match_prog = re.search(r'Processed:\s*(\d+)\s*/\s*(\d+)', subline)
                if match_prog:
                    curr, total = int(match_prog.group(1)), int(match_prog.group(2))
                    if total > 0: self.prod_progress.setValue(int((curr/total)*100))
                    continue
                self.sig_log.emit(subline, False)

    def handle_stderr(self):
        err_data = self.prod_process.readAllStandardError().data().decode("utf8", errors="replace")
        if err_data.strip():
            self.sig_log.emit(err_data, True)
            
    def process_finished(self):
        self.interactive_group.setEnabled(False) # [핵심] 뷰어가 종료되면 다시 비활성화
        self.sig_log.emit("\033[1;32m[PROD]\033[0m Offline Production Process Finished.", False)
        
    def terminate_process(self):
        if self.prod_process.state() == QProcess.Running:
            self.prod_process.terminate()
            self.prod_process.waitForFinished(2000)