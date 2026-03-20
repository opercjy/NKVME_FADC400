#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGroupBox, 
                             QCheckBox, QRadioButton, QLabel, QButtonGroup)
from PyQt5.QtGui import QFont
from PyQt5.QtCore import Qt

class TLUSimulatorWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.init_ui()
        self.update_simulation()

    def init_ui(self):
        main_layout = QHBoxLayout(self)

        # --- [좌측] 입력부 (채널 및 논리 선택) ---
        input_layout = QVBoxLayout()
        
        # 1. 채널 마스킹 선택
        ch_group = QGroupBox("1. Channel Masking (입력 채널 선택)")
        ch_group.setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #2196F3; }")
        ch_layout = QVBoxLayout()
        self.cb_chs = []
        for i in range(4):
            cb = QCheckBox(f"Channel {i} Enable")
            cb.setChecked(True) # 기본적으로 모두 켜둠
            cb.toggled.connect(self.update_simulation)
            self.cb_chs.append(cb)
            ch_layout.addWidget(cb)
        ch_group.setLayout(ch_layout)
        input_layout.addWidget(ch_group)

        # 2. 트리거 논리(Logic) 선택
        logic_group = QGroupBox("2. Coincidence Logic (동시계수 논리)")
        logic_group.setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #4CAF50; }")
        logic_layout = QVBoxLayout()
        self.logic_bg = QButtonGroup(self)
        
        self.rb_or = QRadioButton("OR (하나라도 들어오면 트리거)")
        self.rb_and = QRadioButton("AND (모두 동시에 들어와야 트리거)")
        self.rb_or.setChecked(True)
        
        self.rb_or.toggled.connect(self.update_simulation)
        self.rb_and.toggled.connect(self.update_simulation)
        
        self.logic_bg.addButton(self.rb_or)
        self.logic_bg.addButton(self.rb_and)
        
        logic_layout.addWidget(self.rb_or)
        logic_layout.addWidget(self.rb_and)
        logic_group.setLayout(logic_layout)
        input_layout.addWidget(logic_group)
        input_layout.addStretch()

        main_layout.addLayout(input_layout, stretch=1)

        # --- [우측] 출력부 (시뮬레이션 결과 및 설명) ---
        output_layout = QVBoxLayout()
        
        # 3. 회로도 시각화
        visual_group = QGroupBox("3. Logic Gate Visualization")
        visual_layout = QVBoxLayout()
        self.lbl_visual = QLabel()
        self.lbl_visual.setFont(QFont("Consolas", 14, QFont.Bold))
        self.lbl_visual.setAlignment(Qt.AlignCenter)
        self.lbl_visual.setStyleSheet("background-color: #1E1E1E; color: #00FF00; padding: 20px; border-radius: 10px;")
        visual_layout.addWidget(self.lbl_visual)
        visual_group.setLayout(visual_layout)
        output_layout.addWidget(visual_group, stretch=2)

        # 4. 최종 TLT 코드 및 설명
        result_group = QGroupBox("4. Config File Result (결과 코드)")
        result_group.setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #FF9800; }")
        result_layout = QVBoxLayout()
        
        self.lbl_hex = QLabel("TLT    0    0xFFFE")
        self.lbl_hex.setFont(QFont("Consolas", 20, QFont.Bold))
        self.lbl_hex.setStyleSheet("color: #E91E63;")
        self.lbl_hex.setAlignment(Qt.AlignCenter)
        
        self.lbl_desc = QLabel("설명: 활성화된 채널 중 하나라도 신호가 발생하면 트리거를 생성합니다.")
        self.lbl_desc.setFont(QFont("Arial", 12))
        self.lbl_desc.setAlignment(Qt.AlignCenter)
        self.lbl_desc.setWordWrap(True)

        result_layout.addSpacing(20)
        result_layout.addWidget(self.lbl_hex)
        result_layout.addSpacing(20)
        result_layout.addWidget(self.lbl_desc)
        result_layout.addStretch()
        result_group.setLayout(result_layout)
        output_layout.addWidget(result_group, stretch=1)

        main_layout.addLayout(output_layout, stretch=2)

    def update_simulation(self):
        """ 사용자의 클릭에 따라 회로도와 TLT Hex 코드를 동적으로 계산하여 업데이트 """
        active_chs = [i for i, cb in enumerate(self.cb_chs) if cb.isChecked()]
        is_or = self.rb_or.isChecked()

        # 1. 시각화 (ASCII Logic Gate) 생성
        visual_text = ""
        gate_str = " OR  " if is_or else " AND "
        
        if not active_chs:
            visual_text = "[No Channels Selected]\n\nTrigger Disabled"
            hex_code = "0x0000"
            desc = "경고: 선택된 채널이 없습니다. 트리거가 발생하지 않습니다."
        else:
            for i, ch in enumerate(active_chs):
                if i == len(active_chs) // 2:
                    visual_text += f"[Ch {ch}] ----| {gate_str} |----> [ Trigger OUT ]\n"
                else:
                    visual_text += f"[Ch {ch}] ----|       |\n"

            # 2. TLT Hex 코드 계산 로직 (교육용 표준화)
            if len(active_chs) == 4 and is_or:
                hex_code = "0xFFFE" # Notice FADC 기본 4채널 OR
                desc = "가장 범용적인 세팅입니다. 4개 채널 중 어디서든 신호가 오면 수집합니다."
            elif len(active_chs) == 4 and not is_or:
                hex_code = "0x8000" # Notice FADC 기본 4채널 AND
                desc = "강력한 동시계수 조건입니다. 4개 채널 모두 동시에 신호가 와야 수집합니다. (우주선 판별용)"
            else:
                # 비트 마스킹 계산 (Ch0=Bit0, Ch1=Bit1 ...)
                mask = 0
                for ch in active_chs:
                    mask |= (1 << ch)
                
                # 16진수로 포맷팅 (ex: 0x0001, 0x0005)
                hex_code = f"0x{mask:04X}"
                
                if len(active_chs) == 1:
                    desc = f"단일 채널 트리거입니다. Ch {active_chs[0]}에 신호가 들어올 때만 수집합니다."
                else:
                    mode_str = "중 하나라도" if is_or else "에 모두 동시에"
                    desc = f"비트마스크 조합 트리거입니다. 선택된 채널 {active_chs} {mode_str} 신호가 들어오면 수집합니다."

        self.lbl_visual.setText(visual_text)
        self.lbl_hex.setText(f"TLT    0    {hex_code}")
        self.lbl_desc.setText(f"📘 해설: {desc}\n(위의 붉은색 코드를 config 파일의 TLT 항목에 복사하세요)")
