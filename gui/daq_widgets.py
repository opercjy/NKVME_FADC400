#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import glob
import re
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QPushButton, 
                             QLabel, QLineEdit, QGroupBox, QTextEdit, QComboBox, 
                             QLCDNumber, QProgressBar, QTabWidget, QSpinBox, 
                             QRadioButton, QButtonGroup, QCheckBox)
from PyQt5.QtCore import pyqtSignal, Qt, QRegExp
from PyQt5.QtGui import QRegExpValidator, QTextCursor, QFont

from path_controller import PathControllerWidget

class ConsoleWidget(QGroupBox):
    def __init__(self, parent=None):
        super().__init__("System Console", parent)
        self.last_was_progress = False
        self.init_ui()

    def init_ui(self):
        self.setStyleSheet("""
            QGroupBox { font-weight: bold; border: 2px solid #B0BEC5; border-radius: 8px; margin-top: 20px; }
            QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 15px; padding: 0 5px; color: #455A64; }
        """)
        log_layout = QVBoxLayout(self)
        log_layout.setContentsMargins(15, 25, 15, 15) 
        
        self.lbl_hw_status = QLabel("Hardware Status: Waiting for DAQ...")
        self.lbl_hw_status.setFont(QFont("Consolas", 12, QFont.Bold))
        self.lbl_hw_status.setStyleSheet("background-color: #ECEFF1; color: #E65100; padding: 5px; border-radius: 4px; border: 1px solid #CFD8DC;")
        log_layout.addWidget(self.lbl_hw_status)

        self.log_viewer = QTextEdit()
        self.log_viewer.setReadOnly(True)
        self.log_viewer.setStyleSheet("""
            QTextEdit {
                background-color: #F8F9FA; 
                color: #263238; 
                font-family: 'Consolas', monospace; 
                font-size: 12px; 
                border: 1px solid #CFD8DC;
            }
        """)
        log_layout.addWidget(self.log_viewer)

    def set_hw_status(self, html_text):
        self.lbl_hw_status.setText(html_text)

    def clear(self):
        self.log_viewer.clear()
        self.last_was_progress = False

    def ansi_to_html(self, text):
        text = text.replace('<', '&lt;').replace('>', '&gt;')
        text = re.sub(r'\033\[1;31m(.*?)\033\[0m', r'<span style="color:#D32F2F; font-weight:bold;">\1</span>', text) 
        text = re.sub(r'\033\[1;32m(.*?)\033\[0m', r'<span style="color:#2E7D32; font-weight:bold;">\1</span>', text) 
        text = re.sub(r'\033\[1;33m(.*?)\033\[0m', r'<span style="color:#E65100; font-weight:bold;">\1</span>', text) 
        text = re.sub(r'\033\[1;34m(.*?)\033\[0m', r'<span style="color:#1565C0; font-weight:bold;">\1</span>', text) 
        text = re.sub(r'\033\[1;35m(.*?)\033\[0m', r'<span style="color:#8E24AA; font-weight:bold;">\1</span>', text) 
        text = re.sub(r'\033\[1;36m(.*?)\033\[0m', r'<span style="color:#00838F; font-weight:bold;">\1</span>', text) 
        text = re.sub(r'\033\[1;37m(.*?)\033\[0m', r'<span style="color:#212121; font-weight:bold;">\1</span>', text) 
        text = re.sub(r'\033\[[\d;]*m', '', text) 
        return text

    def print_log(self, text, is_error=False, is_progress=False):
        for line in text.splitlines():
            if not line.strip(): continue
            if is_error:
                clean_line = re.sub(r'\033\[[\d;]*m', '', line)
                safe_line = clean_line.replace('<', '&lt;').replace('>', '&gt;')
                html_line = f'<span style="color:#D32F2F; font-weight:bold;">{safe_line}</span>'
            else:
                html_line = self.ansi_to_html(line)

            cursor = self.log_viewer.textCursor()
            if is_progress:
                if self.last_was_progress:
                    cursor.movePosition(QTextCursor.End)
                    cursor.select(QTextCursor.BlockUnderCursor)
                    cursor.removeSelectedText()
                    cursor.insertBlock() 
                    cursor.insertHtml(html_line)
                else:
                    self.log_viewer.append(html_line)
                self.last_was_progress = True
            else:
                if self.last_was_progress:
                    cursor.movePosition(QTextCursor.End)
                    cursor.insertBlock()
                self.log_viewer.append(html_line)
                self.last_was_progress = False
        self.log_viewer.moveCursor(QTextCursor.End)


class DashboardWidget(QGroupBox):
    def __init__(self, parent=None):
        super().__init__("Live Status Dashboard", parent)
        self.init_ui()

    def init_ui(self):
        self.setStyleSheet("""
            QGroupBox { font-weight: bold; border: 2px solid #9E9E9E; background-color: #FAFAFA; border-radius: 8px; margin-top: 20px; }
            QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 15px; padding: 0 5px; color: #424242; }
        """)
        dash_layout = QVBoxLayout(self)
        dash_layout.setContentsMargins(15, 25, 15, 15) 
        dash_layout.setSpacing(10) 
        
        self.lbl_dash_mode = QLabel("IDLE")
        self.lbl_dash_mode.setFont(QFont("Arial", 14, QFont.Bold)); self.lbl_dash_mode.setStyleSheet("color: #FF9800;")
        mode_layout = QHBoxLayout(); mode_layout.addWidget(QLabel("Current Mode:")); mode_layout.addWidget(self.lbl_dash_mode)
        dash_layout.addLayout(mode_layout)
        
        time_layout = QHBoxLayout()
        self.lbl_start_time = QLabel("Start: --:--:--")
        self.lbl_elapsed_time = QLabel("Elapsed: 00:00:00")
        self.lbl_start_time.setStyleSheet("font-weight: bold; color: #555;")
        self.lbl_elapsed_time.setStyleSheet("font-weight: bold; color: #1976D2;")
        time_layout.addWidget(self.lbl_start_time); time_layout.addWidget(self.lbl_elapsed_time)
        dash_layout.addLayout(time_layout)

        self.lbl_trigger_rate = QLabel("Trigger Rate: 0.0 Hz")
        self.lbl_trigger_rate.setFont(QFont("Arial", 12, QFont.Bold))
        self.lbl_trigger_rate.setStyleSheet("color: #E91E63;") 
        dash_layout.addWidget(self.lbl_trigger_rate)

        dash_layout.addWidget(QLabel("Acquired Events:"))
        self.lcd_events = QLCDNumber(); self.lcd_events.setDigitCount(9)
        self.lcd_events.setSegmentStyle(QLCDNumber.Flat)
        self.lcd_events.setStyleSheet("QLCDNumber { background-color: #F0F4F8; color: #37474F; border: 2px solid #CFD8DC; border-radius: 6px; }")
        self.lcd_events.setMinimumHeight(45) 
        dash_layout.addWidget(self.lcd_events)
        
        h_sys = QHBoxLayout()
        self.lbl_q = QLabel("DataQ: 0")
        self.lbl_q.setStyleSheet("background: #FFF9C4; border: 1px solid #FBC02D; padding: 3px; color: #F57F17; font-weight:bold; border-radius:3px;")
        self.lbl_p = QLabel("Pool: 0")
        self.lbl_p.setStyleSheet("background: #C8E6C9; border: 1px solid #388E3C; padding: 3px; color: #1B5E20; font-weight:bold; border-radius:3px;")
        h_sys.addWidget(self.lbl_q); h_sys.addWidget(self.lbl_p)
        dash_layout.addLayout(h_sys)
        
        dash_layout.addWidget(QLabel("Automation Progress:"))
        self.auto_progress = QProgressBar()
        self.auto_progress.setAlignment(Qt.AlignCenter)
        self.auto_progress.setFixedHeight(20)
        dash_layout.addWidget(self.auto_progress)
        
        self.lbl_disk = QLabel("Disk Space: Calculating...")
        dash_layout.addWidget(self.lbl_disk)
        
        dash_layout.addWidget(QLabel("Current Config Summary:"))
        self.txt_config_summary = QTextEdit(); self.txt_config_summary.setReadOnly(True)
        self.txt_config_summary.setStyleSheet("background-color: #FFFFFF; color: #3E2723; font-size: 11px; border: 1px solid #E0E0E0;")
        dash_layout.addWidget(self.txt_config_summary, stretch=1)

    def set_mode(self, mode_str): self.lbl_dash_mode.setText(mode_str)
    def set_time(self, start_str, elapsed_str):
        self.lbl_start_time.setText(start_str)
        self.lbl_elapsed_time.setText(elapsed_str)
    def update_stats(self, events, rate, dataq, pool):
        self.lcd_events.display(events)
        self.lbl_trigger_rate.setText(f"Trigger Rate: {rate} Hz")
        self.lbl_q.setText(f"DataQ: {dataq}")
        self.lbl_p.setText(f"Pool: {pool}")
    def set_progress(self, max_val, curr_val):
        self.auto_progress.setMaximum(max_val)
        self.auto_progress.setValue(curr_val)
    def set_config_summary(self, text):
        self.txt_config_summary.setText(text)
    def update_disk_space(self, disk_dir):
        import shutil
        try:
            total, used, free = shutil.disk_usage(disk_dir)
            gb_free = free // (2**30)
            self.lbl_disk.setText(f"Disk Free [{os.path.basename(disk_dir)}]: {gb_free} GB")
            self.lbl_disk.setStyleSheet("color: red; font-weight: bold;" if gb_free < 5 else "color: black;")
        except: pass


class DAQControlTabWidget(QWidget):
    sig_start_manual = pyqtSignal(dict)
    sig_start_scan = pyqtSignal(dict)
    sig_start_longrun = pyqtSignal(dict)
    sig_config_summary_req = pyqtSignal(str)

    def __init__(self, data_dir, config_dir, parent=None):
        super().__init__(parent)
        self.data_dir = data_dir
        self.config_dir = config_dir
        self.init_ui()

    def init_ui(self):
        wrapper_layout = QVBoxLayout(self)
        wrapper_layout.setContentsMargins(0, 0, 0, 0)
        
        self.path_ctrl = PathControllerWidget("📁 Current Data Save Path:", self.data_dir)
        wrapper_layout.addWidget(self.path_ctrl)

        # ==============================================================================
        # 💡 [UI 추가] DB 연동 주의사항 박스 추가 (HV 패널과 연동됨을 사용자에게 인지)
        # ==============================================================================
        lbl_hv_link = QLabel("🔗 [DB 연동 안내] 수집(Run) 시작 시 'HV Control' 탭에 설정된 고전압(HV) 값이 E-Log에 자동 기록(Push)됩니다.")
        lbl_hv_link.setStyleSheet("color: #D84315; font-weight: bold; background-color: #FBE9E7; padding: 8px; border-radius: 6px; border: 1px solid #FFAB91; margin-bottom: 5px;")
        wrapper_layout.addWidget(lbl_hv_link)

        inner_tabs = QTabWidget()
        inner_tabs.addTab(self.create_manual_tab(), "🕹️ Manual DAQ")
        inner_tabs.addTab(self.create_scan_tab(), "🔄 THR Scan")
        inner_tabs.addTab(self.create_longrun_tab(), "⏱️ Long Run")
        wrapper_layout.addWidget(inner_tabs)

    def create_manual_tab(self):
        tab = QWidget(); layout = QVBoxLayout(tab)
        layout.setContentsMargins(8, 8, 8, 8); layout.setSpacing(8)
        
        self.chk_enable_mon = QCheckBox("📡 Enable Real-time DQM (Send Data to Monitor)")
        self.chk_enable_mon.setChecked(True)
        self.chk_enable_mon.setStyleSheet("color: #1976D2; font-weight: bold; margin-bottom: 5px;")
        layout.addWidget(self.chk_enable_mon)
        
        self.combo_config = QComboBox()
        self.combo_config.currentIndexChanged.connect(self.emit_config_summary) 
        self.input_runnum = QLineEdit(); self.input_runnum.setValidator(QRegExpValidator(QRegExp("^[0-9]+$")))
        self.combo_runtag = QComboBox()
        self.combo_runtag.addItems(["TEST", "PHYSICS", "CALIBRATION", "COSMIC"])
        self.combo_runtag.setStyleSheet("background-color: #FFF3E0;")
        self.combo_quality = QComboBox()
        self.combo_quality.addItems(["PENDING (미정)", "GOOD (정상)", "BAD (폐기)"])
        self.combo_quality.setStyleSheet("background-color: #E8EAF6;")
        self.input_desc = QLineEdit()
        
        layout.addWidget(QLabel("Base Config File:")); layout.addWidget(self.combo_config)
        row1 = QHBoxLayout()
        row1.addWidget(QLabel("Run:")); row1.addWidget(self.input_runnum)
        row1.addWidget(QLabel("Tag:")); row1.addWidget(self.combo_runtag)
        row1.addWidget(QLabel("Quality:")); row1.addWidget(self.combo_quality)
        layout.addLayout(row1)
        layout.addWidget(QLabel("Description:")); layout.addWidget(self.input_desc)
        
        stop_group = QGroupBox("Stop Condition")
        stop_layout = QHBoxLayout(); stop_layout.setContentsMargins(5, 10, 5, 5)
        self.rb_manual_cont = QRadioButton("Continuous"); self.rb_manual_evt = QRadioButton("By Events"); self.rb_manual_time = QRadioButton("By Time (Sec)")
        self.rb_manual_cont.setChecked(True)
        self.spin_manual_val = QSpinBox(); self.spin_manual_val.setRange(1, 10000000); self.spin_manual_val.setValue(5000); self.spin_manual_val.setEnabled(False)
        self.rb_manual_cont.toggled.connect(lambda: self.spin_manual_val.setEnabled(not self.rb_manual_cont.isChecked()))
        
        stop_layout.addWidget(self.rb_manual_cont); stop_layout.addWidget(self.rb_manual_evt); stop_layout.addWidget(self.rb_manual_time); stop_layout.addWidget(self.spin_manual_val)
        stop_group.setLayout(stop_layout); layout.addWidget(stop_group)

        btn = QPushButton("▶ Start Manual DAQ")
        btn.setStyleSheet("background-color: #4CAF50; color: white; padding: 10px; font-weight:bold; font-size: 13px;")
        btn.clicked.connect(self.on_start_manual)
        layout.addWidget(btn); layout.addStretch()
        return tab

    def create_scan_tab(self):
        tab = QWidget(); layout = QVBoxLayout(tab)
        layout.setContentsMargins(8, 8, 8, 8); layout.setSpacing(6)
        layout.addWidget(QLabel("자동으로 Threshold(THR) 값을 변경하며 스캔합니다."))
        
        row2 = QHBoxLayout()
        self.spin_thr_start = QSpinBox(); self.spin_thr_start.setRange(0, 1000); self.spin_thr_start.setValue(50)
        self.spin_thr_end = QSpinBox(); self.spin_thr_end.setRange(0, 1000); self.spin_thr_end.setValue(200)
        self.spin_thr_step = QSpinBox(); self.spin_thr_step.setRange(1, 500); self.spin_thr_step.setValue(10)
        row2.addWidget(QLabel("Start:")); row2.addWidget(self.spin_thr_start)
        row2.addWidget(QLabel("End:")); row2.addWidget(self.spin_thr_end)
        row2.addWidget(QLabel("Step:")); row2.addWidget(self.spin_thr_step)
        layout.addLayout(row2)
        
        row_opt = QHBoxLayout()
        self.rb_scan_evt = QRadioButton("By Events"); self.rb_scan_time = QRadioButton("By Time (Sec)"); self.rb_scan_evt.setChecked(True)
        row_opt.addWidget(self.rb_scan_evt); row_opt.addWidget(self.rb_scan_time)
        self.spin_scan_val = QSpinBox(); self.spin_scan_val.setRange(1, 10000000); self.spin_scan_val.setValue(5000)
        row_opt.addWidget(self.spin_scan_val)
        row_opt.addWidget(QLabel("Idle(s):"))
        self.spin_scan_rest = QSpinBox(); self.spin_scan_rest.setRange(0, 3600); self.spin_scan_rest.setValue(3)       
        row_opt.addWidget(self.spin_scan_rest)
        layout.addLayout(row_opt)
        
        btn = QPushButton("🔄 Start Threshold Scan")
        btn.setStyleSheet("background-color: #FF9800; color: white; padding: 10px; font-weight:bold;")
        btn.clicked.connect(self.on_start_scan)
        layout.addWidget(btn); layout.addStretch()
        return tab

    def create_longrun_tab(self):
        tab = QWidget(); layout = QVBoxLayout(tab)
        layout.setContentsMargins(8, 8, 8, 8); layout.setSpacing(6)
        layout.addWidget(QLabel("지정된 조건 단위로 파일을 분할 수집합니다."))
        
        row3 = QHBoxLayout()
        self.rb_long_time = QRadioButton("Chunk by Time (Min)"); self.rb_long_evt = QRadioButton("Chunk by Events"); self.rb_long_time.setChecked(True)
        row3.addWidget(self.rb_long_time); row3.addWidget(self.rb_long_evt); layout.addLayout(row3)
        
        row4 = QHBoxLayout()
        self.spin_chunk_val = QSpinBox(); self.spin_chunk_val.setRange(1, 10000000); self.spin_chunk_val.setValue(60)
        self.spin_total_chunk = QSpinBox(); self.spin_total_chunk.setRange(1, 1000); self.spin_total_chunk.setValue(10)
        row4.addWidget(QLabel("Value/Chunk:")); row4.addWidget(self.spin_chunk_val)
        row4.addWidget(QLabel("Total Chunks:")); row4.addWidget(self.spin_total_chunk)
        row4.addWidget(QLabel("Idle(s):"))
        self.spin_long_rest = QSpinBox(); self.spin_long_rest.setRange(0, 3600); self.spin_long_rest.setValue(5)       
        row4.addWidget(self.spin_long_rest)
        layout.addLayout(row4)
        
        btn = QPushButton("⏱️ Start Long Run")
        btn.setStyleSheet("background-color: #009688; color: white; padding: 10px; font-weight:bold;")
        btn.clicked.connect(self.on_start_longrun)
        layout.addWidget(btn); layout.addStretch()
        return tab

    def refresh_configs(self):
        self.combo_config.blockSignals(True)
        self.combo_config.clear()
        if os.path.exists(self.config_dir):
            files = [f for f in os.listdir(self.config_dir) if f.endswith('.config')]
            for f in sorted(files):
                if f != "temp_auto.config": self.combo_config.addItem(f, os.path.join(self.config_dir, f))
        self.combo_config.blockSignals(False)
        self.emit_config_summary()

    def emit_config_summary(self):
        path = self.combo_config.currentData()
        if not path or not os.path.exists(path): return
        summary = ""
        with open(path, 'r') as f:
            for line in f:
                if line.strip().startswith("FADC"): summary += f"[보드] {line.strip()}\n"
                elif line.strip().startswith("NDP"): summary += f"[포인트] {line.strip()}\n"
                elif line.strip().startswith("TLT"): summary += f"[트리거] {line.strip()}\n"
                elif line.strip().startswith("THR"): summary += f"[임계값] {line.strip()}\n"
                elif line.strip().startswith("DLY"): summary += f"[지연] {line.strip()}\n"
                elif line.strip().startswith("POL"): summary += f"[극성] {line.strip()}\n"
                elif line.strip().startswith("CW"): summary += f"[동시계수폭(CW)] {line.strip()}\n"
                elif line.strip().startswith("PWT"): summary += f"[펄스폭(PWT)] {line.strip()}\n"
                elif line.strip().startswith("PCI"): summary += f"[펄스간격(PCI)] {line.strip()}\n"
                elif line.strip().startswith("PCT"): summary += f"[펄스개수(PCT)] {line.strip()}\n"
        self.sig_config_summary_req.emit(summary)

    def set_next_run_number(self, run_num):
        self.input_runnum.setText(run_num)

    def get_common_params(self):
        return {
            "run_num": self.input_runnum.text().strip(),
            "config_path": self.combo_config.currentData(),
            "enable_mon": self.chk_enable_mon.isChecked(),
            "tag": self.combo_runtag.currentText(),
            "quality": self.combo_quality.currentText().split()[0],
            "desc": self.input_desc.text()
        }

    def on_start_manual(self):
        p = self.get_common_params()
        if self.rb_manual_evt.isChecked(): p['stop'] = ('events', self.spin_manual_val.value())
        elif self.rb_manual_time.isChecked(): p['stop'] = ('time', self.spin_manual_val.value())
        else: p['stop'] = ('cont', 0)
        self.sig_start_manual.emit(p)

    def on_start_scan(self):
        p = self.get_common_params()
        p['scan'] = {
            'start': self.spin_thr_start.value(), 'end': self.spin_thr_end.value(), 'step': self.spin_thr_step.value(),
            'stop_type': 'events' if self.rb_scan_evt.isChecked() else 'time',
            'stop_val': self.spin_scan_val.value(), 'rest_sec': self.spin_scan_rest.value()
        }
        self.sig_start_scan.emit(p)

    def on_start_longrun(self):
        p = self.get_common_params()
        p['longrun'] = {
            'chunk_type': 'events' if self.rb_long_evt.isChecked() else 'time',
            'chunk_val': self.spin_chunk_val.value(), 'total_chunks': self.spin_total_chunk.value(),
            'rest_sec': self.spin_long_rest.value()
        }
        self.sig_start_longrun.emit(p)