#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGroupBox, 
                             QComboBox, QLabel)
from PyQt5.QtGui import QFont
from PyQt5.QtCore import Qt

class TLUSimulatorWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.presets = self.init_presets()
        self.init_ui()
        self.update_simulation()

    def get_box_style(self, color_hex):
        return f"""
        QGroupBox {{
            font-family: 'Arial';
            font-weight: bold;
            font-size: 14px;
            border: 2px solid {color_hex};
            border-radius: 8px;
            margin-top: 18px;
            background-color: transparent;
        }}
        QGroupBox::title {{
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 8px;
            left: 15px;
            color: {color_hex};
        }}
        """

    def init_presets(self):
        base_style = "margin:0; line-height:1.4; font-size:14px; font-family:'Consolas', monospace;"
        c0 = "<span style='color:#00E5FF;'>[ Ch 0 ]</span>"
        c1 = "<span style='color:#00E5FF;'>[ Ch 1 ]</span>"
        c2 = "<span style='color:#00E5FF;'>[ Ch 2 ]</span>"
        c3 = "<span style='color:#00E5FF;'>[ Ch 3 ]</span>"
        c_out = "<span style='color:#FF1744; font-weight:bold;'>[ TRIGGER OUT ]</span>"
        
        return [
            {
                "name": "1. Any Channel (Global OR)",
                "desc": "가장 범용적인 세팅입니다. 4개 채널 중 어디서든 신호가 오면 트리거합니다.",
                "logic": lambda c0, c1, c2, c3: c0 or c1 or c2 or c3,
                "art": f"""<pre style="{base_style}">
 {c0} ────┐
 {c1} ────┤
              ├── <span style='color:#FFC107; font-weight:bold;'>OR </span> ──▶ {c_out}
 {c2} ────┤
 {c3} ────┘</pre>"""
            },
            {
                "name": "2. All Channels (Global AND)",
                "desc": "강력한 동시계수 조건입니다. 4개 채널 모두 동시에 신호가 와야 수집합니다. (우주선 판별용)",
                "logic": lambda c0, c1, c2, c3: c0 and c1 and c2 and c3,
                "art": f"""<pre style="{base_style}">
 {c0} ────┐
 {c1} ────┤
              ├── <span style='color:#FFC107; font-weight:bold;'>AND</span> ──▶ {c_out}
 {c2} ────┤
 {c3} ────┘</pre>"""
            },
            {
                "name": "3. (Ch0 OR Ch1) AND (Ch2 AND Ch3)",
                "desc": "박사님 맞춤 논리: 0,1번 중 하나가 들어오고, 동시에 2,3번이 모두 들어와야 트리거합니다.",
                "logic": lambda c0, c1, c2, c3: (c0 or c1) and (c2 and c3),
                "art": f"""<pre style="{base_style}">
 {c0} ──┐
            ├── <span style='color:#FFC107; font-weight:bold;'>OR </span> ──┐
 {c1} ──┘           │
                    ├── <span style='color:#FFC107; font-weight:bold;'>AND</span> ──▶ {c_out}
 {c2} ──┐           │
            ├── <span style='color:#FFC107; font-weight:bold;'>AND</span> ──┘
 {c3} ──┘</pre>"""
            },
            {
                "name": "4. (Ch0 OR Ch1) AND (Ch2 OR Ch3)",
                "desc": "상하/좌우로 묶인 2개의 신틸레이터 패들 쌍이 교차로 동시계수될 때 사용합니다.",
                "logic": lambda c0, c1, c2, c3: (c0 or c1) and (c2 or c3),
                "art": f"""<pre style="{base_style}">
 {c0} ──┐
            ├── <span style='color:#FFC107; font-weight:bold;'>OR </span> ──┐
 {c1} ──┘           │
                    ├── <span style='color:#FFC107; font-weight:bold;'>AND</span> ──▶ {c_out}
 {c2} ──┐           │
            ├── <span style='color:#FFC107; font-weight:bold;'>OR </span> ──┘
 {c3} ──┘</pre>"""
            },
            {
                "name": "5. (Ch0 AND Ch1) OR (Ch2 AND Ch3)",
                "desc": "Ch0,1 번 쌍이나 Ch2,3 번 쌍 중 하나라도 동시계수를 만족하면 트리거합니다.",
                "logic": lambda c0, c1, c2, c3: (c0 and c1) or (c2 and c3),
                "art": f"""<pre style="{base_style}">
 {c0} ──┐
            ├── <span style='color:#FFC107; font-weight:bold;'>AND</span> ──┐
 {c1} ──┘           │
                    ├── <span style='color:#FFC107; font-weight:bold;'>OR </span> ──▶ {c_out}
 {c2} ──┐           │
            ├── <span style='color:#FFC107; font-weight:bold;'>AND</span> ──┘
 {c3} ──┘</pre>"""
            },
            {
                "name": "6. Majority (>= 2 Channels)",
                "desc": "4개의 채널 중 2개 이상의 채널에서 동시에 신호가 발생할 때 트리거합니다.",
                "logic": lambda c0, c1, c2, c3: (c0 + c1 + c2 + c3) >= 2,
                "art": f"""<pre style="{base_style}">
 {c0} ────┐
 {c1} ────┤  <span style='color:#FFC107; font-weight:bold;'>MAJ</span>
              ├── <span style='color:#FFC107; font-weight:bold;'>>=2</span> ──▶ {c_out}
 {c2} ────┤
 {c3} ────┘</pre>"""
            },
            {
                "name": "7. Majority (>= 3 Channels)",
                "desc": "4개의 채널 중 3개 이상의 채널에서 동시에 신호가 발생할 때 트리거합니다.",
                "logic": lambda c0, c1, c2, c3: (c0 + c1 + c2 + c3) >= 3,
                "art": f"""<pre style="{base_style}">
 {c0} ────┐
 {c1} ────┤  <span style='color:#FFC107; font-weight:bold;'>MAJ</span>
              ├── <span style='color:#FFC107; font-weight:bold;'>>=3</span> ──▶ {c_out}
 {c2} ────┤
 {c3} ────┘</pre>"""
            },
            {
                "name": "8. Single Channel 0 Only",
                "desc": "오직 Ch 0에 신호가 들어올 때만 수집합니다. 다른 채널은 무시됩니다.",
                "logic": lambda c0, c1, c2, c3: c0,
                "art": f"""<pre style="{base_style}">
 {c0} ─────────────────▶ {c_out}

 <span style='color:#757575;'>[ Ch 1 ] ──x (Muted)
 [ Ch 2 ] ──x (Muted)
 [ Ch 3 ] ──x (Muted)</span></pre>"""
            }
        ]

    def init_ui(self):
        main_layout = QVBoxLayout(self)

        # --- 상단: 콤보박스 선택부 ---
        select_group = QGroupBox("1. Trigger Logic Selection (트리거 논리 선택)")
        select_group.setStyleSheet(self.get_box_style("#2196F3"))
        select_layout = QVBoxLayout()
        select_layout.setContentsMargins(15, 25, 15, 15)
        
        self.combo_logic = QComboBox()
        self.combo_logic.setStyleSheet("font-size: 15px; padding: 6px; border: 1px solid #B0BEC5; border-radius: 4px;")
        for p in self.presets:
            self.combo_logic.addItem(p["name"])
        
        self.combo_logic.currentIndexChanged.connect(self.update_simulation)
        select_layout.addWidget(self.combo_logic)
        select_group.setLayout(select_layout)
        main_layout.addWidget(select_group)

        # --- 중간/하단을 나누는 HBox ---
        bottom_layout = QHBoxLayout()

        # 2. 회로도 시각화
        visual_group = QGroupBox("2. Logic Circuit Visualization (논리 회로)")
        visual_group.setStyleSheet(self.get_box_style("#4CAF50"))
        
        visual_bg_widget = QWidget()
        visual_bg_widget.setStyleSheet("background-color: #1A1A1A; border-radius: 10px; border: 2px solid #424242;")
        
        bg_layout = QHBoxLayout(visual_bg_widget)
        bg_layout.addStretch()
        
        self.lbl_visual = QLabel()
        self.lbl_visual.setFont(QFont("Consolas", 14, QFont.Bold))
        self.lbl_visual.setAlignment(Qt.AlignLeft | Qt.AlignVCenter) 
        self.lbl_visual.setStyleSheet("background-color: transparent; color: #B0BEC5;")
        bg_layout.addWidget(self.lbl_visual)
        
        bg_layout.addStretch()
        
        visual_layout = QVBoxLayout()
        visual_layout.setContentsMargins(10, 20, 10, 10)
        visual_layout.addWidget(visual_bg_widget)
        visual_group.setLayout(visual_layout)
        
        bottom_layout.addWidget(visual_group, stretch=5)

        # 3. 최종 TLT 코드 및 설명
        result_group = QGroupBox("3. Config File Code (설정 파일 코드)")
        result_group.setStyleSheet(self.get_box_style("#FF9800"))
        result_layout = QVBoxLayout()
        result_layout.setContentsMargins(10, 20, 10, 10)
        
        # [핵심 수정] 초기 텍스트에서 0 제거
        self.lbl_hex = QLabel("TLT    0xFFFE")
        self.lbl_hex.setFont(QFont("Consolas", 22, QFont.Bold))
        self.lbl_hex.setStyleSheet("color: #E91E63; background-color: #FFF3E0; padding: 10px; border-radius: 8px;")
        self.lbl_hex.setAlignment(Qt.AlignCenter)
        
        # [핵심 추가] 텍스트 드래그 및 복사 허용 & 마우스 커서 I-Beam으로 변경
        self.lbl_hex.setTextInteractionFlags(Qt.TextSelectableByMouse)
        self.lbl_hex.setCursor(Qt.IBeamCursor)
        self.lbl_hex.setToolTip("드래그하여 복사 (Ctrl+C) 후 Config 파일에 붙여넣으세요.")
        
        self.lbl_desc = QLabel()
        self.lbl_desc.setFont(QFont("Arial", 11))
        self.lbl_desc.setStyleSheet("color: #37474F;")
        self.lbl_desc.setAlignment(Qt.AlignCenter)
        self.lbl_desc.setWordWrap(True)

        result_layout.addStretch()
        result_layout.addWidget(self.lbl_hex)
        result_layout.addSpacing(15)
        result_layout.addWidget(self.lbl_desc)
        result_layout.addStretch()
        result_group.setLayout(result_layout)
        bottom_layout.addWidget(result_group, stretch=4)

        main_layout.addLayout(bottom_layout)

    def update_simulation(self):
        """ 콤보박스 선택에 따라 진리표를 계산하여 TLT Hex Code 갱신 """
        idx = self.combo_logic.currentIndex()
        preset = self.presets[idx]

        self.lbl_visual.setText(preset["art"])

        tlt_val = 0
        logic_func = preset["logic"]
        
        for pattern in range(16):
            c0 = (pattern >> 0) & 1
            c1 = (pattern >> 1) & 1
            c2 = (pattern >> 2) & 1
            c3 = (pattern >> 3) & 1
            
            if logic_func(c0, c1, c2, c3):
                tlt_val |= (1 << pattern)
                
        hex_code = f"0x{tlt_val:04X}"
        
        # [핵심 수정] 0 보드 번호 지칭 삭제
        self.lbl_hex.setText(f"TLT    {hex_code}")
        self.lbl_desc.setText(f"📘 해설: {preset['desc']}\n\n(위 붉은색 코드를 드래그하여 복사하세요)")