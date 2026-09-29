import json
import uuid

def gen_id():
    return str(uuid.uuid4())

def base_widget(w_type, x, y, w, h, **kwargs):
    widget = {
        "type": w_type,
        "objID": gen_id(),
        "x": x,
        "y": y,
        "width": w,
        "height": h,
        "flags": ["LV_OBJ_FLAG_SCROLLABLE"],
        "localStyles": {"definition": {"MAIN": {"DEFAULT": {}}}},
        "children": []
    }
    
    style_def = widget["localStyles"]["definition"]["MAIN"]["DEFAULT"]
    
    for k, v in kwargs.items():
        if k == "use_style":
            widget["useStyle"] = v
        elif k == "align":
            widget["align"] = f"LV_ALIGN_{v}"
        elif k == "children":
            widget["children"] = v
        elif k == "eventHandlers":
            widget["eventHandlers"] = v
        elif k in ["text", "textType", "longMode", "passwordMode", "placeholder", "oneLineMode", "buttons"]:
            widget[k] = v
        elif k == "identifier":
            widget["name"] = v
        elif k == "clickableFlag":
            widget["flags"].append("LV_OBJ_FLAG_CLICKABLE")
            if "LV_OBJ_FLAG_SCROLLABLE" in widget["flags"]:
                widget["flags"].remove("LV_OBJ_FLAG_SCROLLABLE")
        else:
            style_def[k] = v
            
    if not style_def:
        del widget["localStyles"]
        
    return widget

def make_sidebar(active_idx):
    sidebar = base_widget("LVGLPanelWidget", 0, 0, 120, 320, use_style="PanelTransparent")
    pages = ["Admin_Users", "Admin_History", "Admin_Settings", "Admin_Test"]
    labels = ["Users", "History", "Settings", "Test"]
    
    for i, (p, l) in enumerate(zip(pages, labels)):
        style = "ButtonPrimary" if i == active_idx else "ButtonDefault"
        btn = base_widget("LVGLButtonWidget", 10, 10 + i * 55, 100, 45, use_style=style, clickableFlag=True)
        btn["children"].append(base_widget("LVGLLabelWidget", 0, 0, 100, 45, use_style="LabelDefault", text=l, textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", text_color="ForegroundWhite" if i == active_idx else "TextPrimary", longMode="WRAP", recolor=False, useStaticText=True))
        btn["eventHandlers"] = [{"objID": gen_id(), "eventName": "CLICKED", "handlerType": "action", "action": f"load_{p}", "userData": 0}]
        sidebar["children"].append(btn)
        
    # Logout button
    btn_logout = base_widget("LVGLButtonWidget", 10, -10, 100, 40, use_style="ButtonDanger", align="BOTTOM_LEFT", clickableFlag=True)
    btn_logout["children"].append(base_widget("LVGLLabelWidget", 0, 0, 100, 40, use_style="LabelDefault", text="Keluar", textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", text_color="ForegroundWhite", longMode="WRAP", recolor=False, useStaticText=True))
    btn_logout["eventHandlers"] = [{"objID": gen_id(), "eventName": "CLICKED", "handlerType": "action", "action": "load_Main", "userData": 0}]
    sidebar["children"].append(btn_logout)
    
    return sidebar

data = json.load(open('3inch5-display-rfid-barcode-gui.eez-project', encoding='utf-8'))

# Delete old Admin_Panel and Admin_Password to rebuild them
data['userPages'] = [p for p in data.get('userPages', []) if p['name'] not in ['Admin_Panel', 'Admin_Password', 'Admin_Users', 'Admin_History', 'Admin_Settings', 'Admin_Test']]

def create_page(name, components):
    return {
        "name": name,
        "objID": gen_id(),
        "left": 0, "top": 0, "width": 480, "height": 320,
        "createAtStart": True, "deleteOnScreenUnload": False,
        "connectionLines": [], "localVariables": [], "componentGroups": [], "userProperties": [],
        "components": [{
            "type": "LVGLScreenWidget",
            "objID": gen_id(),
            "useStyle": "ScreenDefault",
            "left": 0, "top": 0, "width": 480, "height": 320,
            "children": components
        }]
    }

# 1. Admin Password
pwd_card = base_widget("LVGLPanelWidget", 0, 0, 360, 280, use_style="Card", align="CENTER")
pwd_lbl = base_widget("LVGLLabelWidget", 0, 10, 360, -1, use_style="LabelDefault", text="Masukkan PIN Admin", textType="literal", text_align="CENTER", text_font="rm26", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)

pwd_input = base_widget("LVGLTextareaWidget", 60, 50, 240, 44, use_style="TextareaDefault", identifier="textarea_input_password")
pwd_input["passwordMode"] = True
pwd_input["placeholder"] = "PIN"
pwd_input["oneLineMode"] = True
pwd_input["text_align"] = "CENTER"

# Use individual buttons for keypad since ButtonMatrix seems unsupported in this way
keypad = base_widget("LVGLPanelWidget", 60, 104, 240, 160, use_style="PanelTransparent")
keys = ["1", "2", "3", "4", "5", "6", "7", "8", "9", "Del", "0", "OK"]
kw, kh = 70, 35
for i, k in enumerate(keys):
    row, col = i // 3, i % 3
    btn = base_widget("LVGLButtonWidget", col*(kw+10), row*(kh+5), kw, kh, use_style="ButtonDefault", clickableFlag=True)
    btn["children"].append(base_widget("LVGLLabelWidget", 0, 0, kw, kh, use_style="LabelDefault", text=k, textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", longMode="WRAP", recolor=False, useStaticText=True))
    btn["eventHandlers"] = [{"objID": gen_id(), "eventName": "CLICKED", "handlerType": "action", "action": f"pwd_btn_{k}_pressed", "userData": 0}]
    keypad["children"].append(btn)

pwd_card["children"].extend([pwd_lbl, pwd_input, keypad])
data['userPages'].append(create_page("Admin_Password", [pwd_card]))

# 2. Admin_Users
u_card = base_widget("LVGLPanelWidget", 130, 10, 340, 300, use_style="Card")
u_title = base_widget("LVGLLabelWidget", 0, 0, 340, -1, use_style="LabelDefault", text="Manajemen Pengguna", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
btn_add = base_widget("LVGLButtonWidget", 0, 30, 100, 36, use_style="ButtonPrimary", clickableFlag=True)
btn_add["children"].append(base_widget("LVGLLabelWidget", 0, 0, 100, 36, use_style="LabelDefault", text="Tambah", textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", text_color="ForegroundWhite", longMode="WRAP", recolor=False, useStaticText=True))
btn_edit = base_widget("LVGLButtonWidget", 110, 30, 100, 36, use_style="ButtonDefault", clickableFlag=True)
btn_edit["children"].append(base_widget("LVGLLabelWidget", 0, 0, 100, 36, use_style="LabelDefault", text="Edit", textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", longMode="WRAP", recolor=False, useStaticText=True))
btn_del = base_widget("LVGLButtonWidget", 220, 30, 100, 36, use_style="ButtonDanger", clickableFlag=True)
btn_del["children"].append(base_widget("LVGLLabelWidget", 0, 0, 100, 36, use_style="LabelDefault", text="Hapus", textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", text_color="ForegroundWhite", longMode="WRAP", recolor=False, useStaticText=True))
# Fake table using list of labels
u_table = base_widget("LVGLPanelWidget", 0, 80, 320, 190, use_style="PanelTransparent")
u_table["children"].append(base_widget("LVGLLabelWidget", 0, 0, 320, -1, use_style="LabelDefault", text="UID | Nama | Tipe", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True))
u_table["children"].append(base_widget("LVGLLabelWidget", 0, 30, 320, -1, use_style="LabelDefault", text="E2345 | Ahmad | RFID\nA1928 | Tamu | QR", textType="literal", text_font="rm14", text_color="TextSecondary", identifier="lbl_user_list", longMode="WRAP", recolor=False, useStaticText=True))
u_card["children"].extend([u_title, btn_add, btn_edit, btn_del, u_table])
data['userPages'].append(create_page("Admin_Users", [make_sidebar(0), u_card]))

# 3. Admin_History
h_card = base_widget("LVGLPanelWidget", 130, 10, 340, 300, use_style="Card")
h_title = base_widget("LVGLLabelWidget", 0, 0, 340, -1, use_style="LabelDefault", text="Riwayat Akses", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
h_table = base_widget("LVGLPanelWidget", 0, 30, 320, 240, use_style="PanelTransparent")
h_table["children"].append(base_widget("LVGLLabelWidget", 0, 0, 320, -1, use_style="LabelDefault", text="Waktu | Nama | Metode", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True))
h_table["children"].append(base_widget("LVGLLabelWidget", 0, 30, 320, -1, use_style="LabelDefault", text="10:05 | Ahmad | RFID\n11:30 | Tamu | QR", textType="literal", text_font="rm14", text_color="TextSecondary", identifier="lbl_history_list", longMode="WRAP", recolor=False, useStaticText=True))
h_card["children"].extend([h_title, h_table])
data['userPages'].append(create_page("Admin_History", [make_sidebar(1), h_card]))

# 4. Admin_Settings
s_card = base_widget("LVGLPanelWidget", 130, 10, 340, 300, use_style="Card")
s_title = base_widget("LVGLLabelWidget", 0, 0, 340, -1, use_style="LabelDefault", text="Pengaturan Sistem", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
lbl_ssid = base_widget("LVGLLabelWidget", 0, 30, 150, -1, use_style="LabelDefault", text="SSID WiFi:", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
inp_ssid = base_widget("LVGLTextareaWidget", 0, 50, 150, 36, use_style="TextareaDefault", identifier="inp_ssid", oneLineMode=True)
lbl_pass = base_widget("LVGLLabelWidget", 0, 96, 150, -1, use_style="LabelDefault", text="Pass WiFi:", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
inp_pass = base_widget("LVGLTextareaWidget", 0, 116, 150, 36, use_style="TextareaDefault", identifier="inp_pass", oneLineMode=True, passwordMode=True)
lbl_ip = base_widget("LVGLLabelWidget", 0, 162, 150, -1, use_style="LabelDefault", text="Static IP:", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
inp_ip = base_widget("LVGLTextareaWidget", 0, 182, 150, 36, use_style="TextareaDefault", identifier="inp_ip", oneLineMode=True)

lbl_ntp = base_widget("LVGLLabelWidget", 160, 30, 150, -1, use_style="LabelDefault", text="NTP Server:", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
inp_ntp = base_widget("LVGLTextareaWidget", 160, 50, 150, 36, use_style="TextareaDefault", identifier="inp_ntp", oneLineMode=True)
lbl_rel = base_widget("LVGLLabelWidget", 160, 96, 150, -1, use_style="LabelDefault", text="Durasi Pintu (detik):", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
slider_rel = base_widget("LVGLSliderWidget", 160, 126, 150, 16, use_style="SliderDefault", identifier="slider_relay")
val_rel = base_widget("LVGLLabelWidget", 160, 152, 150, -1, use_style="LabelDefault", text="5 Detik", textType="literal", text_font="rm14", text_color="TextPrimary", identifier="lbl_relay_val", longMode="WRAP", recolor=False, useStaticText=True)

btn_save = base_widget("LVGLButtonWidget", 160, 182, 150, 36, use_style="ButtonPrimary", clickableFlag=True)
btn_save["children"].append(base_widget("LVGLLabelWidget", 0, 0, 150, 36, use_style="LabelDefault", text="Simpan", textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", text_color="ForegroundWhite", longMode="WRAP", recolor=False, useStaticText=True))

s_card["children"].extend([s_title, lbl_ssid, inp_ssid, lbl_pass, inp_pass, lbl_ip, inp_ip, lbl_ntp, inp_ntp, lbl_rel, slider_rel, val_rel, btn_save])
data['userPages'].append(create_page("Admin_Settings", [make_sidebar(2), s_card]))

# 5. Admin_Test
t_card = base_widget("LVGLPanelWidget", 130, 10, 340, 300, use_style="Card")
t_title = base_widget("LVGLLabelWidget", 0, 0, 340, -1, use_style="LabelDefault", text="Uji Perangkat Keras", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)

btn_t1 = base_widget("LVGLButtonWidget", 0, 40, 120, 44, use_style="ButtonDefault", clickableFlag=True)
btn_t1["children"].append(base_widget("LVGLLabelWidget", 0, 0, 120, 44, use_style="LabelDefault", text="Test Relay", textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", longMode="WRAP", recolor=False, useStaticText=True))
lbl_t1 = base_widget("LVGLLabelWidget", 130, 52, 200, -1, use_style="LabelDefault", text="Status: Menunggu", textType="literal", text_font="rm14", identifier="lbl_test_relay", longMode="WRAP", recolor=False, useStaticText=True)

btn_t2 = base_widget("LVGLButtonWidget", 0, 100, 120, 44, use_style="ButtonDefault", clickableFlag=True)
btn_t2["children"].append(base_widget("LVGLLabelWidget", 0, 0, 120, 44, use_style="LabelDefault", text="Test Kamera", textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", longMode="WRAP", recolor=False, useStaticText=True))
lbl_t2 = base_widget("LVGLLabelWidget", 130, 112, 200, -1, use_style="LabelDefault", text="Status: Menunggu", textType="literal", text_font="rm14", identifier="lbl_test_camera", longMode="WRAP", recolor=False, useStaticText=True)

btn_t3 = base_widget("LVGLButtonWidget", 0, 160, 120, 44, use_style="ButtonDefault", clickableFlag=True)
btn_t3["children"].append(base_widget("LVGLLabelWidget", 0, 0, 120, 44, use_style="LabelDefault", text="Test RFID", textType="literal", text_align="CENTER", align="CENTER", text_font="rm14", longMode="WRAP", recolor=False, useStaticText=True))
lbl_t3 = base_widget("LVGLLabelWidget", 130, 172, 200, -1, use_style="LabelDefault", text="Status: Menunggu", textType="literal", text_font="rm14", identifier="lbl_test_rfid", longMode="WRAP", recolor=False, useStaticText=True)

t_card["children"].extend([t_title, btn_t1, lbl_t1, btn_t2, lbl_t2, btn_t3, lbl_t3])
data['userPages'].append(create_page("Admin_Test", [make_sidebar(3), t_card]))

# Also need to make sure the Main screen action "btn_admin_on_pressed" loads Admin_Password instead of Admin_Panel
# We don't have to change that here if the action was already set, but we might need to change the handler in EEZ studio or C code.
# The user will link the action to Load Page "Admin_Password".

json.dump(data, open('3inch5-display-rfid-barcode-gui.eez-project', 'w', encoding='utf-8'), indent=2)
print("Updated project with explicit Admin Screens (no tabview/table)")
