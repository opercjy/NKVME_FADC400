#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import re
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QPushButton, 
                             QLabel, QGroupBox, QLineEdit, QFileDialog, QProgressBar, 
                             QMessageBox, QCheckBox, QInputDialog)
from PyQt5.QtCore import pyqtSignal, QProcess, Qt

from path_controller import PathControllerWidget

EXE_PRODUCTION = "production_nfadc400"

class ProductionManagerWidget(QWidget):
    # 💡 [핵심] 시그널 인자 3개로 확장: (text: str, is_error: bool, is_progress: bool)
    sig_log = pyqtSignal(str, bool, bool) 
    sig_prod_done = pyqtSignal(dict) 

    def __init__(self, current_dir, default_data_dir, parent=None):
        super().__init__(parent)
        self.current_dir = current_dir
        self.data_dir = default_data_dir
        self.prod_stats = {} 
        
        self.prod_process = QProcess(self)
        self.prod_process.readyReadStandardOutput.connect(self.handle_stdout)
        self.prod_process.readyReadStandardError.connect(self.handle_stderr)
        self.prod_process.finished.connect(self.process_finished)

        self.init_ui()

    def init_ui(self):
        main_layout = QVBoxLayout(self)
        
        self.path_ctrl = PathControllerWidget("📁 Batch Output Path:", self.data_dir)
        main_layout.addWidget(self.path_ctrl)
        
        file_group = QGroupBox("1. Select Raw Data File (.root)")
        file_group.setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #673AB7; border-radius: 8px; margin-top: 5px; } QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 15px; color: #673AB7; }")
        file_layout = QHBoxLayout(); file_layout.setContentsMargins(15, 25, 15, 15)
        
        self.input_prod_file = QLineEdit()
        self.input_prod_file.setReadOnly(True)
        btn_browse = QPushButton("📂 Browse Raw File...")
        btn_browse.clicked.connect(self.browse_prod_file)
        
        file_layout.addWidget(self.input_prod_file, stretch=1)
        file_layout.addWidget(btn_browse)
        file_group.setLayout(file_layout)
        main_layout.addWidget(file_group)

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

        self.interactive_group = QGroupBox("3. Interactive Viewer Controls (-d Mode)")
        self.interactive_group.setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #FF9800; border-radius: 8px; margin-top: 15px; } QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 15px; color: #FF9800; }")
        self.interactive_group.setEnabled(False) 
        
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
        
        if self.prod_process.state() == QProcess.Running:
            self.prod_process.kill()
            self.prod_process.waitForFinished(1000)
            
        target_dir = self.path_ctrl.get_path()
        out_filename = os.path.basename(infile).replace(".root", "_prod.root")
        outfile = os.path.join(target_dir, out_filename)
        
        self.prod_stats = {
            'raw_file': os.path.basename(infile),
            'prod_file': out_filename,
            'mode': 'Waveform SAVE' if self.chk_save_wave.isChecked() else 'Normal (No Wave)',
            'events': 0, 'elapsed': 0.0, 'speed': 0.0
        }
        
        self.interactive_group.setEnabled(False)
        self.prod_progress.setValue(0)
        self.sig_log.emit(f"\033[1;35m[PROD]\033[0m Starting Batch Analysis: {os.path.basename(infile)} -> {outfile}", False, False)
        
        args = ["-i", infile, "-o", outfile]
        if self.chk_save_wave.isChecked(): args.append("-w")
            
        self.prod_process.start(os.path.join(self.current_dir, EXE_PRODUCTION), args)

    def run_prod_interactive(self):
        infile = self.input_prod_file.text().strip()
        if not infile or not os.path.exists(infile):
            QMessageBox.warning(self, "Error", "선택된 파일이 없거나 존재하지 않습니다.")
            return
            
        if self.prod_process.state() == QProcess.Running:
            self.prod_process.kill()
            self.prod_process.waitForFinished(1000)
            
        self.interactive_group.setEnabled(True) 
        self.sig_log.emit(f"\033[1;35m[PROD]\033[0m Launching Interactive Viewer (-d): {os.path.basename(infile)}", False, False)
        self.sig_log.emit(f"\033[1;33m[TIP]\033[0m 아래의 'Interactive Viewer Controls' 패널 버튼을 클릭하여 파형을 넘겨보세요.", False, False)
        
        self.prod_process.start(os.path.join(self.current_dir, EXE_PRODUCTION), ["-i", infile, "-d"])

    def send_command(self, cmd):
        if self.prod_process.state() == QProcess.Running:
            self.prod_process.write((cmd + "\n").encode('utf-8'))

    def cmd_jump(self):
        if self.prod_process.state() != QProcess.Running: return
        num, ok = QInputDialog.getInt(self, "Jump to Event", "이동할 이벤트 번호를 입력하세요:", 0, 0, 2000000000, 1)
        if ok:
            self.send_command(f"j\n{num}")

    def handle_stdout(self):
        raw_data = self.prod_process.readAllStandardOutput().data().decode("utf8", errors="replace")
        for line in raw_data.split('\n'):
            for subline in line.split('\r'):
                if not subline.strip(): continue
                clean_line = re.sub(r'\033\[[\d;]*m', '', subline)
                
                # 💡 [버그 픽스] 정규식을 C++ 문자열에 정확히 매칭시켜 덮어쓰기 플래그(is_progress=True) 작동
                match_prog = re.search(r'Processing Dump:\s*(\d+)\s*/\s*(\d+)', clean_line)
                if match_prog:
                    curr, total = int(match_prog.group(1)), int(match_prog.group(2))
                    if total > 0: self.prod_progress.setValue(int((curr/total)*100))
                    # 💡 3번째 인자(is_progress=True)를 넘겨서 콘솔에 오버라이트 하도록 지시
                    self.sig_log.emit(subline, False, True) 
                    continue
                
                m_ev = re.search(r'Total Processed\s*:\s*(\d+)', clean_line)
                if m_ev: self.prod_stats['events'] = int(m_ev.group(1))
                
                m_time = re.search(r'Total Elapsed Time\s*:\s*([0-9.]+)', clean_line)
                if m_time: self.prod_stats['elapsed'] = float(m_time.group(1))
                
                m_spd = re.search(r'Average Speed\s*:\s*([0-9.]+)', clean_line)
                if m_spd: self.prod_stats['speed'] = float(m_spd.group(1))
                
                self.sig_log.emit(subline, False, False)

    def handle_stderr(self):
        err_data = self.prod_process.readAllStandardError().data().decode("utf8", errors="replace")
        if err_data.strip():
            self.sig_log.emit(err_data, True, False)
            
    def process_finished(self):
        self.interactive_group.setEnabled(False) 
        self.sig_log.emit("\033[1;32m[PROD]\033[0m Offline Production Process Finished.", False, False)
        if self.prod_stats:
            self.sig_prod_done.emit(self.prod_stats)
            self.prod_stats = {}
        
    def terminate_process(self):
        if self.prod_process.state() == QProcess.Running:
            self.prod_process.kill() 
            self.prod_process.waitForFinished(2000)