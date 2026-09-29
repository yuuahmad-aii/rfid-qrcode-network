import json
import uuid
import shutil
import copy

TARGET_FILE = "3inch5-display-rfid-barcode-gui.eez-project"
REF_FILE = "TrailCurrentFireside.eez-project"

def gen_id():
    return str(uuid.uuid4())

def default_style():
    return {
        "objID": gen_id(),
        "useStyle": "default",
        "conditionalStyles": [],
        "childStyles": []
    }

def base_widget(w_type, left, top, width, height, use_style="default", **kwargs):
    widget = {
        "objID": gen_id(),
        "type": w_type,
        "left": left,
        "top": top,
        "width": width,
        "height": height,
        "customInputs": [],
        "customOutputs": [],
        "style": {
            "objID": gen_id(),
            "useStyle": use_style,
            "conditionalStyles": [],
            "childStyles": []
        },
        "timeline": [],
        "eventHandlers": [],
        "leftUnit": "px",
        "topUnit": "px",
        "widthUnit": "px" if width != -1 else "content",
        "heightUnit": "px" if height != -1 else "content",
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
    
    local_styles_main = {}
    
    for k, v in kwargs.items():
        if k == 'align':
            local_styles_main["align"] = v
        elif k == 'text_align':
            local_styles_main["text_align"] = v
        elif k == 'text_font':
            local_styles_main["text_font"] = v
        elif k == 'text_color':
            local_styles_main["text_color"] = v
        elif k == 'bg_color':
            local_styles_main["bg_color"] = v
        elif k == 'radius':
            local_styles_main["radius"] = v
        else:
            widget[k] = v
            
    if local_styles_main:
        widget["localStyles"]["definition"] = {"MAIN": {"DEFAULT": local_styles_main}}
        
    return widget

def rebuild():
    shutil.copyfile(TARGET_FILE, TARGET_FILE + ".bak")
    
    with open(TARGET_FILE, "r", encoding="utf-8") as f:
        target = json.load(f)
        
    with open(REF_FILE, "r", encoding="utf-8") as f:
        ref = json.load(f)
        
    # Merge visual assets from ref to target
    target["colors"] = copy.deepcopy(ref.get("colors", []))
    target["themes"] = copy.deepcopy(ref.get("themes", []))
    target["fonts"] = copy.deepcopy(ref.get("fonts", []))
    target["bitmaps"] = copy.deepcopy(ref.get("bitmaps", []))
    
    # Enable Dark Theme by default to match reference look better if needed, or stick to Dark
    target["settings"]["general"]["darkTheme"] = True 

    # Merge all reference styles
    ref_styles = ref.get("lvglStyles", {}).get("styles", [])
    target["lvglStyles"] = {
        "objID": gen_id(),
        "styles": copy.deepcopy(ref_styles),
        "defaultStyles": ref.get("lvglStyles", {}).get("defaultStyles", {})
    }
    
    pages = {p['name']: p for p in target.get('userPages', [])}
    
    # ---- 1. Main Screen ----
    if "Main" in pages:
        main_scr = pages["Main"]["components"][0]
        main_scr["children"] = []
        # Main background (use BgBody)
        main_scr["useStyle"] = "ScreenDefault"
        
        # Header Panel
        header = base_widget("LVGLPanelWidget", 0, 0, 480, 44, use_style="PanelTopBar", align="TOP_LEFT")
        clock = base_widget("LVGLLabelWidget", 16, 0, -1, -1, use_style="LabelDefault", text="12:00:00 - 01 Jan 2026", textType="literal", align="LEFT_MID", text_font="rm14", text_color="TextSecondary", longMode="WRAP", recolor=False, useStaticText=True)
        icons = base_widget("LVGLLabelWidget", -16, 0, -1, -1, use_style="LabelDefault", text="\uf1eb   \uf0c2", textType="literal", align="RIGHT_MID", text_font="fa14", text_color="TextSecondary", longMode="WRAP", recolor=False, useStaticText=True)
        header["children"].extend([clock, icons])
        
        # Main Card
        card = base_widget("LVGLPanelWidget", 40, 70, 400, 200, use_style="Card", align="TOP_LEFT")
        title = base_widget("LVGLLabelWidget", 0, 40, 400, -1, use_style="LabelDefault", text="Akses Masuk Ruangan", textType="literal", text_align="CENTER", text_font="rm26", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
        subtitle = base_widget("LVGLLabelWidget", 0, 80, 400, -1, use_style="LabelDefault", text="Silahkan Tempelkan Kartu RFID\natau Arahkan QR Code ke Kamera", textType="literal", text_align="CENTER", text_font="rm14", text_color="TextSecondary", longMode="WRAP", recolor=False, useStaticText=True)
        # Removed the small explicit button. We will make the main card long-pressable.
        card["eventHandlers"].append({"objID": gen_id(), "eventName": "LONG_PRESSED", "handlerType": "action", "action": "btn_admin_on_pressed", "userData": 0})
        
        card["children"].extend([title, subtitle])
        main_scr["children"].extend([header, card])

    # ---- 2. Access Accepted Screen ----
    if "Access_Accepted" in pages:
        acc_scr = pages["Access_Accepted"]["components"][0]
        acc_scr["children"] = []
        acc_scr["useStyle"] = "ScreenDefault"
        
        card = base_widget("LVGLPanelWidget", 40, 40, 400, 240, use_style="Card", bg_color="Success")
        icon = base_widget("LVGLLabelWidget", 0, 30, 400, -1, use_style="LabelDefault", text="\uf084", textType="literal", text_align="CENTER", text_font="fa24", text_color="ForegroundWhite", longMode="WRAP", recolor=False, useStaticText=True)
        title = base_widget("LVGLLabelWidget", 0, 80, 400, -1, use_style="LabelDefault", text="Akses Diterima", textType="literal", text_align="CENTER", text_font="rm26", text_color="ForegroundWhite", longMode="WRAP", recolor=False, useStaticText=True)
        name = base_widget("LVGLLabelWidget", 0, 120, 400, -1, use_style="LabelDefault", text="Selamat Datang, [Nama]", textType="literal", text_align="CENTER", text_font="rm14", text_color="ForegroundWhite", identifier="label_acc_name", longMode="WRAP", recolor=False, useStaticText=True)
        
        bar = base_widget("LVGLBarWidget", 0, -30, 320, 12, use_style="BarDefault", align="BOTTOM_MID", identifier="bar_door")
        bar["value"] = 100
        bar["minValue"] = 0
        bar["maxValue"] = 100
        
        card["children"].extend([icon, title, name, bar])
        acc_scr["children"].append(card)

    # ---- 3. Access Rejected Screen ----
    if "Access_Rejected" in pages:
        rej_scr = pages["Access_Rejected"]["components"][0]
        rej_scr["children"] = []
        rej_scr["useStyle"] = "ScreenDefault"
        
        card = base_widget("LVGLPanelWidget", 40, 40, 400, 240, use_style="Card", bg_color="Danger")
        icon = base_widget("LVGLLabelWidget", 0, 40, 400, -1, use_style="LabelDefault", text="\uf023", textType="literal", text_align="CENTER", text_font="fa24", text_color="ForegroundWhite", longMode="WRAP", recolor=False, useStaticText=True)
        title = base_widget("LVGLLabelWidget", 0, 90, 400, -1, use_style="LabelDefault", text="Akses Ditolak", textType="literal", text_align="CENTER", text_font="rm26", text_color="ForegroundWhite", longMode="WRAP", recolor=False, useStaticText=True)
        reason = base_widget("LVGLLabelWidget", 0, 140, 400, -1, use_style="LabelDefault", text="Kartu Tidak Terdaftar", textType="literal", text_align="CENTER", text_font="rm14", text_color="ForegroundWhite", identifier="label_rej_reason", longMode="WRAP", recolor=False, useStaticText=True)
        
        card["children"].extend([icon, title, reason])
        rej_scr["children"].append(card)

    # ---- 4. Admin Password Screen ----
    if "Admin_Password" in pages:
        pwd_scr = pages["Admin_Password"]["components"][0]
        pwd_scr["children"] = []
        pwd_scr["useStyle"] = "ScreenDefault"
        
        card = base_widget("LVGLPanelWidget", 60, 20, 360, 280, use_style="Card")
        lbl = base_widget("LVGLLabelWidget", 0, 10, 360, -1, use_style="LabelDefault", text="Masukkan PIN Admin", textType="literal", text_align="CENTER", text_font="rm26", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
        
        pwd_input = base_widget("LVGLTextareaWidget", 60, 50, 240, 44, use_style="TextareaDefault", identifier="textarea_input_password")
        pwd_input["passwordMode"] = True
        pwd_input["placeholder"] = "PIN"
        pwd_input["oneLineMode"] = True
        pwd_input["text_align"] = "CENTER"
        
        keypad = base_widget("LVGLButtonMatrixWidget", 60, 104, 240, 160, use_style="ButtonDefault", identifier="button_matrix_password")
        keypad["buttons"] = [
            {"text": "1", "width": 1, "newLine": False}, {"text": "2", "width": 1, "newLine": False}, {"text": "3", "width": 1, "newLine": False},
            {"text": "4", "width": 1, "newLine": True}, {"text": "5", "width": 1, "newLine": False}, {"text": "6", "width": 1, "newLine": False},
            {"text": "7", "width": 1, "newLine": True}, {"text": "8", "width": 1, "newLine": False}, {"text": "9", "width": 1, "newLine": False},
            {"text": "Del", "width": 1, "newLine": True}, {"text": "0", "width": 1, "newLine": False}, {"text": "OK", "width": 1, "newLine": False}
        ]
        keypad["eventHandlers"].append({"objID": gen_id(), "eventName": "CLICKED", "handlerType": "action", "action": "button_matrix_password_pressed", "userData": 0})
        
        card["children"].extend([lbl, pwd_input, keypad])
        pwd_scr["children"].append(card)

    # ---- 5. Admin Panel ----
    if "Admin_Panel" in pages:
        adm_scr = pages["Admin_Panel"]["components"][0]
        adm_scr["children"] = []
        adm_scr["useStyle"] = "ScreenDefault"
        
        tabview = base_widget("LVGLTabViewWidget", 0, 0, 480, 320, use_style="PanelTransparent", identifier="admin_tabview")
        tabview["tabs"] = [
            {"objID": gen_id(), "name": "Users", "components": []},
            {"objID": gen_id(), "name": "History", "components": []},
            {"objID": gen_id(), "name": "Settings", "components": []},
            {"objID": gen_id(), "name": "Test", "components": []}
        ]
        
        # Tab 1: Users
        u_tab = tabview["tabs"][0]
        btn_add = base_widget("LVGLButtonWidget", 10, 10, 100, 36, use_style="ButtonPrimary", clickableFlag=True)
        btn_add["children"].append(base_widget("LVGLLabelWidget", 0, 0, 100, 36, use_style="LabelDefault", text="Tambah", textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", text_color="ForegroundWhite", longMode="WRAP", recolor=False, useStaticText=True))
        btn_edit = base_widget("LVGLButtonWidget", 120, 10, 100, 36, use_style="ButtonDefault", clickableFlag=True)
        btn_edit["children"].append(base_widget("LVGLLabelWidget", 0, 0, 100, 36, use_style="LabelDefault", text="Edit", textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", longMode="WRAP", recolor=False, useStaticText=True))
        btn_del = base_widget("LVGLButtonWidget", 230, 10, 100, 36, use_style="ButtonDanger", clickableFlag=True)
        btn_del["children"].append(base_widget("LVGLLabelWidget", 0, 0, 100, 36, use_style="LabelDefault", text="Hapus", textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", text_color="ForegroundWhite", longMode="WRAP", recolor=False, useStaticText=True))
        
        table_users = base_widget("LVGLTableWidget", 10, 56, 460, 200, use_style="Card", identifier="table_users")
        u_tab["components"].extend([btn_add, btn_edit, btn_del, table_users])
        
        # Tab 2: History
        h_tab = tabview["tabs"][1]
        table_hist = base_widget("LVGLTableWidget", 10, 10, 460, 240, use_style="Card", identifier="table_history")
        h_tab["components"].append(table_hist)
        
        # Tab 3: Settings
        s_tab = tabview["tabs"][2]
        # Layout left column
        lbl_net = base_widget("LVGLLabelWidget", 10, 10, 200, -1, use_style="LabelDefault", text="SSID WiFi:", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
        inp_ssid = base_widget("LVGLTextareaWidget", 10, 30, 200, 36, use_style="TextareaDefault", identifier="inp_ssid", oneLineMode=True)
        lbl_pass = base_widget("LVGLLabelWidget", 10, 76, 200, -1, use_style="LabelDefault", text="Password WiFi:", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
        inp_pass = base_widget("LVGLTextareaWidget", 10, 96, 200, 36, use_style="TextareaDefault", identifier="inp_pass", oneLineMode=True, passwordMode=True)
        lbl_ip = base_widget("LVGLLabelWidget", 10, 142, 200, -1, use_style="LabelDefault", text="Static IP:", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
        inp_ip = base_widget("LVGLTextareaWidget", 10, 162, 200, 36, use_style="TextareaDefault", identifier="inp_ip", oneLineMode=True)
        
        # Layout right column
        lbl_ntp = base_widget("LVGLLabelWidget", 240, 10, 200, -1, use_style="LabelDefault", text="NTP Server:", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
        inp_ntp = base_widget("LVGLTextareaWidget", 240, 30, 200, 36, use_style="TextareaDefault", identifier="inp_ntp", oneLineMode=True)
        lbl_rel = base_widget("LVGLLabelWidget", 240, 76, 200, -1, use_style="LabelDefault", text="Durasi Pintu Terbuka:", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
        slider_rel = base_widget("LVGLSliderWidget", 240, 106, 200, 16, use_style="SliderDefault", identifier="slider_relay")
        val_rel = base_widget("LVGLLabelWidget", 240, 130, 200, -1, use_style="LabelDefault", text="5 Detik", textType="literal", text_font="rm14", text_color="TextPrimary", identifier="lbl_relay_val", longMode="WRAP", recolor=False, useStaticText=True)
        
        btn_save = base_widget("LVGLButtonWidget", 240, 160, 200, 36, use_style="ButtonPrimary", clickableFlag=True)
        btn_save["children"].append(base_widget("LVGLLabelWidget", 0, 0, 200, 36, use_style="LabelDefault", text="Simpan Pengaturan", textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", text_color="ForegroundWhite", longMode="WRAP", recolor=False, useStaticText=True))
        
        s_tab["components"].extend([lbl_net, inp_ssid, lbl_pass, inp_pass, lbl_ip, inp_ip, lbl_ntp, inp_ntp, lbl_rel, slider_rel, val_rel, btn_save])
        
        # Tab 4: Test
        t_tab = tabview["tabs"][3]
        btn_t1 = base_widget("LVGLButtonWidget", 20, 20, 120, 44, use_style="ButtonDefault", clickableFlag=True)
        btn_t1["children"].append(base_widget("LVGLLabelWidget", 0, 0, 120, 44, use_style="LabelDefault", text="Test Relay", textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", longMode="WRAP", recolor=False, useStaticText=True))
        lbl_t1 = base_widget("LVGLLabelWidget", 160, 32, 200, -1, use_style="LabelDefault", text="Status: Menunggu", textType="literal", text_font="rm14", identifier="lbl_test_relay", longMode="WRAP", recolor=False, useStaticText=True)
        
        btn_t2 = base_widget("LVGLButtonWidget", 20, 80, 120, 44, use_style="ButtonDefault", clickableFlag=True)
        btn_t2["children"].append(base_widget("LVGLLabelWidget", 0, 0, 120, 44, use_style="LabelDefault", text="Test Kamera", textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", longMode="WRAP", recolor=False, useStaticText=True))
        lbl_t2 = base_widget("LVGLLabelWidget", 160, 92, 200, -1, use_style="LabelDefault", text="Status: Menunggu", textType="literal", text_font="rm14", identifier="lbl_test_camera", longMode="WRAP", recolor=False, useStaticText=True)
        
        btn_t3 = base_widget("LVGLButtonWidget", 20, 140, 120, 44, use_style="ButtonDefault", clickableFlag=True)
        btn_t3["children"].append(base_widget("LVGLLabelWidget", 0, 0, 120, 44, use_style="LabelDefault", text="Test RFID", textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", longMode="WRAP", recolor=False, useStaticText=True))
        lbl_t3 = base_widget("LVGLLabelWidget", 160, 152, 200, -1, use_style="LabelDefault", text="Status: Menunggu", textType="literal", text_font="rm14", identifier="lbl_test_rfid", longMode="WRAP", recolor=False, useStaticText=True)
        
        t_tab["components"].extend([btn_t1, lbl_t1, btn_t2, lbl_t2, btn_t3, lbl_t3])
        
        adm_scr["children"].append(tabview)

    # Make sure text_font rb24 and rm14 exist in fonts if missing?
    # Reference has it in fonts. Let's just trust they are exported.

    with open(TARGET_FILE, "w", encoding="utf-8") as f:
        json.dump(target, f, indent=2)

if __name__ == "__main__":
    rebuild()
    print("Done")
