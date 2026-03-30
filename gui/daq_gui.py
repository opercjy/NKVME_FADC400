#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import shutil
import glob
import re
from datetime import datetime
from PyQt5.QtWidgets import QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QLabel, QGroupBox, QMessageBox, QTabWidget, QSplitter
from PyQt5.QtCore import QProcess, Qt, QTimer
from PyQt5.QtGui import QFont

import daq_widgets
from hv_control import HVControlPanel
from tlu_simulator import TLUSimulatorWidget
from config_manager import ConfigManagerWidget
from prod_manager import ProductionManagerWidget
from elog_manager import ELogbookWidget 

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(CURRENT_DIR) 

EXE_FRONTEND = "frontend_nfadc400"
EXE_MONITOR = "OnlineMonitor_nfadc400"

class DAQControlCenter(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("6UVME+NFADC400 Central Control")
        self.setMinimumSize(1250, 900)
        
        if len(sys.argv) > 1 and not sys.argv[1].startswith('-'):
            self.workspace_dir = os.path.abspath(sys.argv[1])
        else:
            self.workspace_dir = os.path.join(PROJECT_ROOT, "data")
            
        os.makedirs(self.workspace_dir, exist_ok=True)
        self.data_output_dir = self.workspace_dir
        
        self.config_dir = os.path.join(self.workspace_dir, "config")
        self.temp_config = os.path.join(self.config_dir, "temp_auto.config")
        self.db_path = os.path.join(self.workspace_dir, "daq_history.db")
        
        os.makedirs(self.config_dir, exist_ok=True)
        if not glob.glob(os.path.join(self.config_dir, "*.config")):
            src_cfg = os.path.join(PROJECT_ROOT, "config")
            if os.path.exists(src_cfg):
                for f in glob.glob(os.path.join(src_cfg, "*.config")): shutil.copy(f, self.config_dir)
        
        self.daq_start_timestamp = None
        self.final_events = "0"
        self.final_time = "0.0"
        self.final_rate = "0.0"
        
        self.daq_process = QProcess(self)
        self.daq_process.readyReadStandardOutput.connect(self.handle_stdout)
        self.daq_process.readyReadStandardError.connect(self.handle_stderr)
        self.daq_process.finished.connect(self.process_finished)
        
        self.monitor_process = QProcess(self)
        self.monitor_process.readyReadStandardOutput.connect(self.handle_mon_stdout)
        self.monitor_process.readyReadStandardError.connect(self.handle_mon_stderr)

        self.auto_mode = "NONE" 
        self.scan_queue = []; self.scan_current_val = 0
        self.current_base_runnum = ""; self.current_subrun_idx = 1; self.subrun_max_idx = 1
        self.current_prefix = "run"
        
        self.scan_timer = QTimer(self)
        self.scan_timer.timeout.connect(self.trigger_scan_timeout)
        self.subrun_timer = QTimer(self)
        self.subrun_timer.timeout.connect(self.trigger_subrun_rotation)
        self.manual_timer = QTimer(self)
        self.manual_timer.timeout.connect(self.trigger_manual_timeout)

        self.init_ui()

        self.clock_timer = QTimer(self)
        self.clock_timer.timeout.connect(self.update_clock)
        self.clock_timer.start(1000)

    def init_ui(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)

        top_layout = QHBoxLayout()
        title_lbl = QLabel(f"6UVME+NFADC400 Central Control [Workspace: {os.path.basename(self.workspace_dir)}]")
        title_lbl.setFont(QFont("Arial", 18, QFont.Bold))
        self.clock_lbl = QLabel("0000-00-00 00:00:00")
        self.clock_lbl.setFont(QFont("Consolas", 14, QFont.Bold))
        top_layout.addWidget(title_lbl); top_layout.addStretch(); top_layout.addWidget(self.clock_lbl)
        main_layout.addLayout(top_layout)

        self.middle_splitter = QSplitter(Qt.Horizontal)
        
        left_widget = QWidget(); left_panel = QVBoxLayout(left_widget)
        left_panel.setContentsMargins(0, 0, 5, 0)
        self.tabs = QTabWidget()
        
        self.daq_tab = daq_widgets.DAQControlTabWidget(self.data_output_dir, self.config_dir)
        self.tabs.addTab(self.daq_tab, "🚀 DAQ Controls")
        
        self.hv_panel = HVControlPanel()
        self.tabs.addTab(self.hv_panel, "⚡ HV Control")
        self.tlu_panel = TLUSimulatorWidget()
        self.tabs.addTab(self.tlu_panel, "🎯 TLU Simulator")
        self.config_panel = ConfigManagerWidget(self.config_dir)
        self.tabs.addTab(self.config_panel, "📁 Config & Path")
        self.prod_panel = ProductionManagerWidget(CURRENT_DIR, self.data_output_dir)
        self.tabs.addTab(self.prod_panel, "📦 Offline Production")
        self.elog_panel = ELogbookWidget(self.db_path)
        self.tabs.addTab(self.elog_panel, "🗄️ Database & E-Log")

        left_panel.addWidget(self.tabs, stretch=1)

        btn_group = QGroupBox("System Master Controls")
        btn_layout = QHBoxLayout(); btn_layout.setContentsMargins(5, 5, 5, 5)
        self.btn_start_mon = QPushButton("👁️ Show Monitor")
        self.btn_start_mon.setStyleSheet("background-color: #2196F3; color: white; font-weight: bold; padding: 8px;")
        self.btn_start_mon.clicked.connect(self.start_monitor)
        self.btn_stop_mon = QPushButton("🙈 Hide Monitor")
        self.btn_stop_mon.setStyleSheet("background-color: #9E9E9E; color: white; font-weight: bold; padding: 8px;")
        self.btn_stop_mon.clicked.connect(self.stop_monitor)
        self.btn_clear_mon = QPushButton("🔄 Clear Monitor")
        self.btn_clear_mon.setStyleSheet("background-color: #00BCD4; color: white; font-weight: bold; padding: 8px;")
        self.btn_clear_mon.clicked.connect(self.clear_monitor)
        self.btn_toggle_dash = QPushButton("▶ Hide Dashboard")
        self.btn_toggle_dash.setStyleSheet("background-color: #607D8B; color: white; font-weight: bold; padding: 8px;")
        self.btn_toggle_dash.clicked.connect(self.toggle_dashboard)
        self.btn_stop = QPushButton("🛑 ABORT / STOP")
        self.btn_stop.setStyleSheet("background-color: #E53935; color: white; font-weight: bold; padding: 8px;")
        self.btn_stop.clicked.connect(self.force_stop_daq)
        
        btn_layout.addWidget(self.btn_start_mon); btn_layout.addWidget(self.btn_stop_mon)
        btn_layout.addWidget(self.btn_clear_mon); btn_layout.addWidget(self.btn_toggle_dash); btn_layout.addWidget(self.btn_stop)
        btn_group.setLayout(btn_layout); left_panel.addWidget(btn_group)

        self.dashboard = daq_widgets.DashboardWidget()
        self.middle_splitter.addWidget(left_widget)
        self.middle_splitter.addWidget(self.dashboard)
        self.middle_splitter.setStretchFactor(0, 7); self.middle_splitter.setStretchFactor(1, 3)
        main_layout.addWidget(self.middle_splitter, stretch=1)

        self.console = daq_widgets.ConsoleWidget()
        main_layout.addWidget(self.console, stretch=2) 

        self.daq_tab.sig_start_manual.connect(self.handle_start_manual)
        self.daq_tab.sig_start_scan.connect(self.handle_start_scan)
        self.daq_tab.sig_start_longrun.connect(self.handle_start_longrun)
        self.daq_tab.path_ctrl.sig_path_changed.connect(self.update_data_dir)
        self.daq_tab.sig_config_summary_req.connect(self.dashboard.set_config_summary)
        
        self.hv_panel.sig_log.connect(self.console.print_log)
        self.config_panel.sig_config_updated.connect(self.daq_tab.refresh_configs)
        self.config_panel.sig_config_dir_changed.connect(self.update_config_dir)
        self.prod_panel.sig_log.connect(self.console.print_log)
        self.prod_panel.sig_prod_done.connect(self.handle_prod_done) 
        self.elog_panel.sig_log.connect(self.console.print_log)

        self.daq_tab.refresh_configs()
        self.daq_tab.set_next_run_number(self.elog_panel.get_next_run_number())

    def toggle_dashboard(self):
        if self.dashboard.isVisible():
            self.dashboard.hide()
            self.btn_toggle_dash.setText("◀ Show Dashboard")
            self.setMinimumWidth(700); self.resize(700, self.height())     
        else:
            self.dashboard.show()
            self.btn_toggle_dash.setText("▶ Hide Dashboard")
            self.setMinimumWidth(1250); self.resize(1250, self.height())    

    def update_data_dir(self, new_dir):
        self.data_output_dir = new_dir
        self.console.print_log(f"\033[1;36m[SYSTEM]\033[0m Data output directory changed to: {self.data_output_dir}")

    def update_config_dir(self, new_dir):
        self.config_dir = new_dir
        self.temp_config = os.path.join(self.config_dir, "temp_auto.config")
        self.daq_tab.config_dir = new_dir
        self.daq_tab.refresh_configs()
        self.console.print_log(f"\033[1;36m[SYSTEM]\033[0m Config directory changed to: {self.config_dir}")

    def handle_prod_done(self, stats):
        if stats.get('events', 0) > 0:
            self.elog_panel.record_production(
                stats['raw_file'], stats['prod_file'], stats['mode'],
                stats['events'], stats['elapsed'], stats['speed']
            )

    def start_monitor(self):
        if self.monitor_process.state() != QProcess.Running:
            self.monitor_process.start(os.path.join(CURRENT_DIR, EXE_MONITOR), [])
            self.console.print_log("\033[1;36m[MONITOR]\033[0m Online Display Process Started.")
            
    def stop_monitor(self):
        if self.monitor_process.state() == QProcess.Running:
            self.monitor_process.terminate()
            self.console.print_log("\033[1;36m[MONITOR]\033[0m Online Display hidden/stopped.")

    def clear_monitor(self):
        if self.monitor_process.state() == QProcess.Running:
            self.monitor_process.write(b"c\n")
            self.console.print_log("\033[1;36m[MONITOR]\033[0m Histograms cleared by user command.")

    def update_clock(self):
        self.clock_lbl.setText(datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
        self.dashboard.update_disk_space(self.data_output_dir)
        if self.daq_process.state() == QProcess.Running and self.daq_start_timestamp:
            elapsed = datetime.now() - self.daq_start_timestamp
            hours, remainder = divmod(elapsed.seconds, 3600)
            minutes, seconds = divmod(remainder, 60)
            self.dashboard.set_time(self.lbl_start_time_cache, f"Elapsed: {hours:02d}:{minutes:02d}:{seconds:02d}")

    # =========================================================================
    # DAQ 제어 로직 (Controller)
    # =========================================================================
    def pre_start_check(self, p):
        if not p['run_num']: QMessageBox.warning(self, "Warning", "Run Number를 입력하세요!"); return False
        if not p['config_path']: QMessageBox.warning(self, "Warning", "Config 파일을 선택하세요!"); return False
        free = shutil.disk_usage(self.data_output_dir)[2] // (2**30)
        if free < 2: QMessageBox.critical(self, "Disk Full", f"해당 디스크 용량이 2GB 미만입니다!"); return False
        
        self.daq_start_timestamp = datetime.now()
        self.lbl_start_time_cache = f"Start: {self.daq_start_timestamp.strftime('%H:%M:%S')}"
        self.dashboard.set_time(self.lbl_start_time_cache, "Elapsed: 00:00:00")
        self.dashboard.update_stats(0, "0.0", 0, 0)
        self.final_events = "0"; self.final_time = "0.0"; self.final_rate = "0.0"
        self.tabs.setEnabled(False); self.console.clear()
        return True

    def handle_start_manual(self, p):
        if not self.pre_start_check(p): return
        self.auto_mode = "NONE"
        self.current_prefix = p['prefix']
        
        # 💡 [핵심 패치] 하드코딩된 'run_'을 모두 제거하고 file_base 조합으로 대체
        file_base = f"{self.current_prefix}_{p['run_num']}"
        self.dashboard.set_mode(f"MANUAL [{file_base}]")
        
        hv_volt = self.hv_panel.get_current_hv()
        self.current_run_id = self.elog_panel.record_run(file_base, "MANUAL", p['tag'], hv_volt, p['quality'], p['desc'])
        
        out_root = os.path.join(self.data_output_dir, f"{file_base}.root") 
        args = ["-f", p['config_path'], "-o", out_root]
        if p['enable_mon']: args.append("-d")
        
        stop_type, stop_val = p['stop']
        if stop_type == 'events':
            args.extend(["-n", str(stop_val)])
            self.console.print_log(f"\033[1;36m[SYSTEM]\033[0m Manual DAQ will stop after {stop_val} events.")
            self.daq_process.start(os.path.join(CURRENT_DIR, EXE_FRONTEND), args)
        elif stop_type == 'time':
            self.console.print_log(f"\033[1;36m[SYSTEM]\033[0m Manual DAQ will stop after {stop_val} seconds.")
            self.daq_process.start(os.path.join(CURRENT_DIR, EXE_FRONTEND), args)
            self.manual_timer.start(stop_val * 1000)
        else:
            self.console.print_log(f"\033[1;36m[SYSTEM]\033[0m Manual DAQ running continuously. (Press ABORT to end)")
            self.daq_process.start(os.path.join(CURRENT_DIR, EXE_FRONTEND), args)

    def generate_scan_config(self, base_cfg, target_thr):
        with open(base_cfg, 'r') as f: lines = f.readlines()
        with open(self.temp_config, 'w') as f:
            for line in lines:
                if line.strip().startswith("THR"):
                    parts = line.split()
                    f.write(f"THR    {parts[1]}    " + "    ".join([str(target_thr)] * (len(parts)-2)) + "\n")
                else: f.write(line)
        return self.temp_config

    def handle_start_scan(self, p):
        if not self.pre_start_check(p): return
        self.auto_mode = "SCAN"
        self.current_prefix = p['prefix']
        self.current_base_runnum = p['run_num']
        self.scan_params = p
        
        sp = p['scan']
        self.scan_queue = list(range(sp['start'], sp['end'] + 1, sp['step']))
        if not self.scan_queue: return
        self.dashboard.set_progress(len(self.scan_queue), 0)
        self.run_next_scan_step()

    def run_next_scan_step(self):
        if not self.scan_queue: self.auto_finish(); return
        self.scan_current_val = self.scan_queue.pop(0)
        sp = self.scan_params['scan']
        self.dashboard.set_progress(self.dashboard.auto_progress.maximum(), self.dashboard.auto_progress.maximum() - len(self.scan_queue))
        
        cfg_path = self.generate_scan_config(self.scan_params['config_path'], self.scan_current_val)
        
        # 💡 [핵심 패치]
        file_base = f"{self.current_prefix}_{self.current_base_runnum}_THR{self.scan_current_val}"
        self.dashboard.set_mode(f"SCAN [THR={self.scan_current_val}]")
        
        hv_volt = self.hv_panel.get_current_hv()
        self.current_run_id = self.elog_panel.record_run(file_base, "SCAN", "CALIBRATION", hv_volt, "PENDING", f"Auto Scan: THR={self.scan_current_val}")
        out_root = os.path.join(self.data_output_dir, f"{file_base}.root") 
        
        args = ["-f", cfg_path, "-o", out_root]
        if self.scan_params['enable_mon']: args.append("-d")
            
        if sp['stop_type'] == 'events':
            args.extend(["-n", str(sp['stop_val'])])
            self.console.print_log(f"\n\033[1;36m[AUTO]\033[0m Scan Step: THR={self.scan_current_val} ({sp['stop_val']} Events)")
            self.daq_process.start(os.path.join(CURRENT_DIR, EXE_FRONTEND), args)
        else:
            self.console.print_log(f"\n\033[1;36m[AUTO]\033[0m Scan Step: THR={self.scan_current_val} ({sp['stop_val']} Sec timeout)")
            self.daq_process.start(os.path.join(CURRENT_DIR, EXE_FRONTEND), args)
            self.scan_timer.start(sp['stop_val'] * 1000)

    def handle_start_longrun(self, p):
        if not self.pre_start_check(p): return
        self.auto_mode = "SUBRUN"
        self.current_prefix = p['prefix']
        self.current_base_runnum = p['run_num']
        self.longrun_params = p
        
        self.current_subrun_idx = 1
        self.subrun_max_idx = p['longrun']['total_chunks']
        self.dashboard.set_progress(self.subrun_max_idx, 0)
        self.run_next_subrun()

    def run_next_subrun(self):
        if self.current_subrun_idx > self.subrun_max_idx: self.auto_finish(); return
        self.dashboard.set_progress(self.subrun_max_idx, self.current_subrun_idx)
        
        # 💡 [핵심 패치]
        file_base = f"{self.current_prefix}_{self.current_base_runnum}_part{self.current_subrun_idx:02d}"
        self.dashboard.set_mode(f"LONG RUN [{self.current_subrun_idx}/{self.subrun_max_idx}]")
        
        hv_volt = self.hv_panel.get_current_hv()
        self.current_run_id = self.elog_panel.record_run(file_base, "LONG_RUN", "PHYSICS", hv_volt, "PENDING", f"Chunk {self.current_subrun_idx}/{self.subrun_max_idx}")
        out_root = os.path.join(self.data_output_dir, f"{file_base}.root")
        
        args = ["-f", self.longrun_params['config_path'], "-o", out_root]
        if self.longrun_params['enable_mon']: args.append("-d")
            
        lp = self.longrun_params['longrun']
        if lp['chunk_type'] == 'events':
            args.extend(["-n", str(lp['chunk_val'])])
            self.console.print_log(f"\n\033[1;36m[AUTO]\033[0m Long Run Chunk {self.current_subrun_idx} ({lp['chunk_val']} Events)...")
            self.daq_process.start(os.path.join(CURRENT_DIR, EXE_FRONTEND), args)
        else:
            self.console.print_log(f"\n\033[1;36m[AUTO]\033[0m Long Run Chunk {self.current_subrun_idx} ({lp['chunk_val']} Min timeout)...")
            self.daq_process.start(os.path.join(CURRENT_DIR, EXE_FRONTEND), args)
            self.subrun_timer.start(lp['chunk_val'] * 60 * 1000)

    # =========================================================================
    # 하위 프로세스 통신 및 종료 제어
    # =========================================================================
    def trigger_manual_timeout(self):
        self.manual_timer.stop()
        self.console.print_log("\033[1;36m[AUTO]\033[0m Manual DAQ target time reached. Graceful shutdown...", is_error=True)
        self.daq_process.terminate()

    def trigger_scan_timeout(self):
        self.scan_timer.stop()
        self.console.print_log("\033[1;36m[AUTO]\033[0m Scan time reached. Force saving...", is_error=True)
        self.daq_process.terminate()

    def trigger_subrun_rotation(self):
        self.subrun_timer.stop()
        self.console.print_log("\033[1;36m[AUTO]\033[0m Chunk time reached. Rotating...", is_error=True)
        self.daq_process.terminate() 

    def force_stop_daq(self):
        self.auto_mode = "NONE"; self.scan_queue.clear()
        self.subrun_timer.stop(); self.scan_timer.stop(); self.manual_timer.stop()
        if self.daq_process.state() == QProcess.Running:
            self.console.print_log("\033[1;31m[SYSTEM] ABORTING DAQ gracefully...\033[0m", is_error=True)
            self.daq_process.terminate()
        self.hv_panel.force_shutdown()

    def auto_finish(self):
        self.auto_mode = "NONE"
        self.dashboard.set_mode("IDLE")
        self.tabs.setEnabled(True)
        self.daq_tab.set_next_run_number(self.elog_panel.get_next_run_number())
        self.console.print_log("\n\033[1;32m[SYSTEM]\033[0m === Sequences Completed Successfully! ===")
        self.dashboard.lbl_trigger_rate.setText("Trigger Rate: 0.0 Hz")
        self.daq_start_timestamp = None

    def process_finished(self):
        if hasattr(self, 'current_run_id'):
            status_str = "COMPLETED"
            if hasattr(self, 'final_events') and self.final_events != "0":
                status_str = f"COMPLETED | {self.final_events} evts | {self.final_time}s | {self.final_rate}Hz"
            self.elog_panel.update_status(self.current_run_id, status_str)
        
        self.console.set_hw_status("Hardware Status: Waiting for DAQ...")
        
        if self.auto_mode == "SCAN":
            rest_sec = self.scan_params['scan']['rest_sec']
            if self.scan_queue: 
                self.console.print_log(f"\033[1;33m[AUTO]\033[0m Hardware cooldown... Next scan will start in {rest_sec} seconds.")
                self.dashboard.set_mode("SCAN [RESTING]")
            QTimer.singleShot(max(1000, rest_sec * 1000), self.run_next_scan_step)
            
        elif self.auto_mode == "SUBRUN":
            self.current_subrun_idx += 1
            rest_sec = self.longrun_params['longrun']['rest_sec']
            if self.current_subrun_idx <= self.subrun_max_idx:
                self.console.print_log(f"\033[1;33m[AUTO]\033[0m Hardware cooldown... Next chunk will start in {rest_sec} seconds.")
                self.dashboard.set_mode("LONG RUN [RESTING]")
            QTimer.singleShot(max(1000, rest_sec * 1000), self.run_next_subrun)
            
        elif self.auto_mode == "NONE":
            self.auto_finish()

    def handle_mon_stdout(self):
        raw_data = self.monitor_process.readAllStandardOutput().data().decode("utf8", errors="replace")
        for line in raw_data.split('\n'):
            for subline in line.split('\r'): 
                if subline.strip(): self.console.print_log(f"\033[1;34m[MONITOR]\033[0m {subline.strip()}")

    def handle_mon_stderr(self):
        raw_data = self.monitor_process.readAllStandardError().data().decode("utf8", errors="replace")
        for line in raw_data.split('\n'):
            for subline in line.split('\r'): 
                if subline.strip(): self.console.print_log(f"\033[1;31m[MON ERROR]\033[0m {subline.strip()}", is_error=True)

    def handle_stdout(self):
        raw_data = self.daq_process.readAllStandardOutput().data().decode("utf8", errors="replace")
        for line in raw_data.split('\n'):
            for subline in line.split('\r'): 
                if not subline.strip(): continue
                clean_line = re.sub(r'\033\[[\d;]*m', '', subline)
                
                match_rate = re.search(r'Rate:\s*([0-9.]+)\s*Hz', clean_line)
                if match_rate: self.final_rate = match_rate.group(1)
                match_time = re.search(r'Time:\s*([0-9.]+)\s*s', clean_line)
                if match_time: self.final_time = match_time.group(1)
                match_ev = re.search(r'Events:\s*(\d+)', clean_line)
                if match_ev: self.final_events = match_ev.group(1)
                
                match_q = re.search(r'DataQ:\s*(\d+)', clean_line)
                match_p = re.search(r'Pool:\s*(\d+)', clean_line)
                
                if match_ev and match_rate and match_q and match_p:
                    self.dashboard.update_stats(int(match_ev.group(1)), match_rate.group(1), int(match_q.group(1)), int(match_p.group(1)))
                    self.console.set_hw_status(self.console.ansi_to_html(subline))

                match_sum_ev = re.search(r'Total Events\s*:\s*(\d+)', clean_line)
                if match_sum_ev: self.final_events = match_sum_ev.group(1)
                match_sum_time = re.search(r'Total Elapsed Time\s*:\s*([0-9.]+)\s*sec', clean_line)
                if match_sum_time: self.final_time = match_sum_time.group(1)
                match_sum_rate = re.search(r'Average Trigger Rate\s*:\s*([0-9.]+)\s*Hz', clean_line)
                if match_sum_rate: self.final_rate = match_sum_rate.group(1)

                is_progress_line = "Events:" in clean_line and "Time:" in clean_line
                self.console.print_log(subline, is_error=False, is_progress=is_progress_line)

    def handle_stderr(self):
        self.console.print_log(self.daq_process.readAllStandardError().data().decode("utf8", errors="replace"), is_error=True)

    def closeEvent(self, event):
        if self.daq_process.state() == QProcess.Running or (hasattr(self.hv_panel, 'device') and self.hv_panel.device is not None):
            reply = QMessageBox.question(self, 'System Alert', "DAQ or HV active. Stop and exit?", QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if reply == QMessageBox.Yes:
                self.force_stop_daq()
                if self.daq_process.state() == QProcess.Running: self.daq_process.waitForFinished(3000) 
                if self.monitor_process.state() == QProcess.Running: self.monitor_process.terminate()
                self.prod_panel.terminate_process() 
                event.accept()
            else: event.ignore()
        else:
            if self.monitor_process.state() == QProcess.Running: self.monitor_process.terminate()
            self.prod_panel.terminate_process() 
            event.accept()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = DAQControlCenter()
    window.show()
    sys.exit(app.exec_())