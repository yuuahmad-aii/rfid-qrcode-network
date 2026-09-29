import json
import uuid
import copy
import shutil

FILE_PATH = "c:\\Users\\ahmad\\Documents\\Project-Yuuahmad\\rfid-qrcode-network\\GUI\\3inch5-display-rfid-barcode-gui.eez-project"

def gen_id():
    return str(uuid.uuid4())

def default_style():
    return {
        "objID": gen_id(),
        "useStyle": "default",
        "conditionalStyles": [],
        "childStyles": []
    }

def base_widget(w_type, left, top, width, height, **kwargs):
    widget = {
        "objID": gen_id(),
        "type": w_type,
        "left": left,
        "top": top,
        "width": width,
        "height": height,
        "customInputs": [],
        "customOutputs": [],
        "style": default_style(),
        "timeline": [],
        "eventHandlers": [],
        "leftUnit": "px",
        "topUnit": "px",
        "widthUnit": "px",
        "heightUnit": "px" if not kwargs.get('heightUnit') else kwargs.get('heightUnit'),
        "children": [],
        "widgetFlags": "CLICK_FOCUSABLE|GESTURE_BUBBLE|PRESS_LOCK|SCROLLABLE|SCROLL_CHAIN_HOR|SCROLL_CHAIN_VER|SCROLL_ELASTIC|SCROLL_MOMENTUM|SCROLL_WITH_ARROW|SNAPPABLE",
        "hiddenFlagType": "literal",
        "clickableFlagType": "literal",
        "flagScrollbarMode": "",
        "flagScrollDirection": "",
        "scrollSnapX": "",
        "scrollSnapY": "",
        "checkedStateType": "literal",
        "disabledStateType": "literal",
        "states": "",
        "localStyles": {
            "objID": gen_id(),
            "definition": {}
        },
        "group": "",
        "groupIndex": 0
    }
    for k, v in kwargs.items():
        if k == 'local_styles_def':
            widget['localStyles']['definition'] = v
        else:
            widget[k] = v
    return widget

def create_label(text, left, top, width, height, font=None, color=None, align=None, identifier=""):
    main_default = {}
    if font:
        main_default["text_font"] = font
    if color:
        main_default["text_color"] = color
    if align:
        main_default["text_align"] = align
        
    local_styles = {}
    if main_default:
        local_styles = {"MAIN": {"DEFAULT": main_default}}
        
    return base_widget(
        "LVGLLabelWidget", left, top, width, height,
        text=text, textType="literal", longMode="WRAP", recolor=False, useStaticText=True, identifier=identifier,
        local_styles_def=local_styles,
        widthUnit="content" if width == -1 else "px",
        heightUnit="content" if height == -1 else "px"
    )

def create_panel(left, top, width, height, bg_color=None, border_width=0, identifier=""):
    main_default = {}
    if bg_color:
        main_default["bg_color"] = bg_color
    main_default["border_width"] = border_width
    
    local_styles = {}
    if main_default:
        local_styles = {"MAIN": {"DEFAULT": main_default}}
        
    return base_widget(
        "LVGLPanelWidget", left, top, width, height,
        identifier=identifier,
        local_styles_def=local_styles
    )

def create_button(text, left, top, width, height, identifier=""):
    btn = base_widget("LVGLButtonWidget", left, top, width, height, identifier=identifier, clickableFlag=True)
    label = create_label(text, 0, 0, width, height, align="CENTER")
    btn['children'].append(label)
    return btn

def rebuild():
    shutil.copyfile(FILE_PATH, FILE_PATH + ".bak")
    with open(FILE_PATH, "r") as f:
        data = json.load(f)
        
    pages = {p['name']: p for p in data.get('userPages', [])}
    
    # 1. Main Screen Redesign
    if "Main" in pages:
        main_scr = pages["Main"]["components"][0]
        main_scr["children"] = [] # Clear existing
        
        # Header Panel
        header = create_panel(0, 0, 480, 40, bg_color="FF2C3E50", identifier="header_panel")
        
        # Clock & Date
        clock = create_label("12:00:00 - 01 Jan 2026", 10, 10, -1, -1, font="MONTSERRAT_16", identifier="label_clock")
        header["children"].append(clock)
        
        # Status Icons
        icons = create_label("\xef\x87\xab WiFi   \xef\x83\x9b DB", 320, 10, -1, -1, font="MONTSERRAT_16", identifier="label_status")
        header["children"].append(icons)
        
        main_scr["children"].append(header)
        
        # Center instruction
        title = create_label("Sistem Akses RFID & QR", 0, 90, 480, -1, font="MONTSERRAT_28", align="CENTER")
        subtitle = create_label("Silahkan Tempelkan Kartu atau Arahkan QR Code ke Kamera", 0, 150, 480, -1, font="MONTSERRAT_16", align="CENTER")
        main_scr["children"].extend([title, subtitle])
        
        # Hidden Admin Button
        admin_btn = create_button("Admin", 370, 270, 100, 40, identifier="btn_admin")
        admin_btn["children"][0]["eventHandlers"].append({
            "objID": gen_id(), "eventName": "CLICKED", "handlerType": "action", "action": "btn_admin_on_pressed", "userData": 0
        })
        main_scr["children"].append(admin_btn)

    # 2. Access Accepted Screen
    if "Access_Accepted" in pages:
        acc_scr = pages["Access_Accepted"]["components"][0]
        acc_scr["children"] = []
        
        bg = create_panel(0, 0, 480, 320, bg_color="FF27AE60")
        icon = create_label("\xef\x80\x8c", 0, 50, 480, -1, font="MONTSERRAT_48", align="CENTER", color="FFFFFFFF")
        title = create_label("Akses Diterima", 0, 120, 480, -1, font="MONTSERRAT_32", align="CENTER", color="FFFFFFFF")
        name = create_label("Selamat Datang, [Nama]", 0, 180, 480, -1, font="MONTSERRAT_24", align="CENTER", color="FFFFFFFF", identifier="label_acc_name")
        
        # Progress Bar for door
        bar = base_widget("LVGLBarWidget", 90, 260, 300, 20, identifier="bar_door")
        bar["value"] = 100
        bar["minValue"] = 0
        bar["maxValue"] = 100
        
        bg["children"].extend([icon, title, name, bar])
        acc_scr["children"].append(bg)

    # 3. Access Rejected Screen
    if "Access_Rejected" in pages:
        rej_scr = pages["Access_Rejected"]["components"][0]
        rej_scr["children"] = []
        
        bg = create_panel(0, 0, 480, 320, bg_color="FFE74C3C")
        icon = create_label("\xef\x80\x8d", 0, 60, 480, -1, font="MONTSERRAT_48", align="CENTER", color="FFFFFFFF")
        title = create_label("Akses Ditolak", 0, 130, 480, -1, font="MONTSERRAT_32", align="CENTER", color="FFFFFFFF")
        reason = create_label("Kartu Tidak Terdaftar", 0, 190, 480, -1, font="MONTSERRAT_24", align="CENTER", color="FFFFFFFF", identifier="label_rej_reason")
        
        bg["children"].extend([icon, title, reason])
        rej_scr["children"].append(bg)

    # 4. Admin Panel
    if "Admin_Panel" in pages:
        adm_scr = pages["Admin_Panel"]["components"][0]
        adm_scr["children"] = []
        
        # Tabview
        tabview = base_widget("LVGLTabViewWidget", 0, 0, 480, 320, identifier="admin_tabview")
        tabview["tabs"] = [
            {"objID": gen_id(), "name": "Users", "components": []},
            {"objID": gen_id(), "name": "History", "components": []},
            {"objID": gen_id(), "name": "Settings", "components": []},
            {"objID": gen_id(), "name": "Test", "components": []}
        ]
        
        # Tab 1: Users
        users_tab = tabview["tabs"][0]
        btn_add = create_button("Tambah User", 10, 10, 120, 40)
        table_users = base_widget("LVGLTableWidget", 10, 60, 460, 200, identifier="table_users")
        users_tab["components"].extend([btn_add, table_users])
        
        # Tab 2: History
        hist_tab = tabview["tabs"][1]
        table_hist = base_widget("LVGLTableWidget", 10, 10, 460, 250, identifier="table_history")
        hist_tab["components"].append(table_hist)
        
        # Tab 3: Settings
        set_tab = tabview["tabs"][2]
        lbl_net = create_label("Network Settings", 10, 10, 200, -1)
        btn_net = create_button("Configure WiFi", 10, 40, 150, 40)
        lbl_rel = create_label("Relay Duration (s)", 10, 100, 200, -1)
        slider_rel = base_widget("LVGLSliderWidget", 10, 130, 200, 20)
        set_tab["components"].extend([lbl_net, btn_net, lbl_rel, slider_rel])
        
        # Tab 4: Test
        t_tab = tabview["tabs"][3]
        btn_t1 = create_button("Test Relay", 10, 10, 120, 40)
        btn_t2 = create_button("Test Kamera", 150, 10, 120, 40)
        btn_t3 = create_button("Test RFID", 290, 10, 120, 40)
        t_tab["components"].extend([btn_t1, btn_t2, btn_t3])
        
        adm_scr["children"].append(tabview)

    # Make sure MONTSERRAT fonts are imported
    montserrat_sizes = [16, 24, 28, 32, 48]
    for size in montserrat_sizes:
        font_name = f"lv_font_montserrat_{size}"
        if not any(f.get("name") == font_name for f in data.get("fonts", [])):
             # We just rely on built in LVGL fonts being enabled in lv_conf.h
             pass

    with open(FILE_PATH, "w") as f:
        json.dump(data, f, indent=2)

if __name__ == "__main__":
    rebuild()
    print("Done")
