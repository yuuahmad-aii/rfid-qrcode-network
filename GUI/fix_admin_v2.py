import json
import uuid
import copy

def gen_id():
    return str(uuid.uuid4())

def base_widget(w_type, x, y, w, h, **kwargs):
    widget = {
        "type": w_type,
        "objID": gen_id(),
        "left": x,
        "top": y,
        "width": w,
        "height": h,
        "leftUnit": "px",
        "topUnit": "px",
        "widthUnit": "content" if w == -1 else "px",
        "heightUnit": "content" if h == -1 else "px",
        "customInputs": [],
        "customOutputs": [],
        "timeline": [],
        "eventHandlers": [],
        "children": [],
        "widgetFlags": "",
        "hiddenFlagType": "literal",
        "clickableFlag": False,
        "clickableFlagType": "literal",
        "flagScrollbarMode": "",
        "flagScrollDirection": "",
        "scrollSnapX": "",
        "scrollSnapY": "",
        "group": "",
        "groupIndex": 0,
        "text": "",
        "textType": "literal",
        "placeholder": "",
        "passwordMode": False,
        "min": 0,
        "minType": "literal",
        "max": 100,
        "maxType": "literal",
        "value": 0,
        "valueType": "literal",
        "valueStart": 0,
        "valueStartType": "literal",
        "mode": "NORMAL",
        "enableAnimation": False,
        "maxTextLength": 0,
        "recolor": False,
        "longMode": "WRAP",
        "checkedStateType": "literal",
        "disabledStateType": "literal",
        "states": "",
        "style": {
            "objID": gen_id(),
            "useStyle": "default",
            "conditionalStyles": [],
            "childStyles": []
        },
        "localStyles": {"objID": gen_id(), "definition": {"MAIN": {"DEFAULT": {}}}}
    }
    
    style_def = widget["localStyles"]["definition"]["MAIN"]["DEFAULT"]
    
    for k, v in kwargs.items():
        if k == "use_style":
            widget["useStyle"] = v
        elif k == "align":
            style_def["align"] = v
        elif k == "children":
            widget["children"] = v
        elif k == "eventHandlers":
            widget["eventHandlers"] = v
        elif k in ["text", "textType", "longMode", "passwordMode", "placeholder", "oneLineMode", "buttons", "bg_color"]:
            widget[k] = v
        elif k == "identifier":
            widget["identifier"] = v
        elif k == "clickableFlag":
            widget["clickableFlag"] = v
            if v:
                widget["widgetFlags"] = "CLICKABLE|CLICK_FOCUSABLE"
        else:
            style_def[k] = v
            
    if not style_def:
        # Some widgets need a style even if empty, but let's keep it safe
        pass
        
    return widget

def make_sidebar(active_idx):
    sidebar = base_widget("LVGLPanelWidget", 0, 0, 120, 320, use_style="PanelTransparent")
    pages = ["Admin_Users", "Admin_History", "Admin_Settings", "Admin_Test"]
    labels = ["Users", "History", "Settings", "Test"]
    icons = ["\uf0c0", "\uf017", "\uf013", "\uf1de"]
    
    for i, (p, l, ic) in enumerate(zip(pages, labels, icons)):
        style = "ButtonPrimary" if i == active_idx else "ButtonDefault"
        btn = base_widget("LVGLButtonWidget", 5, 10 + i * 50, 110, 40, use_style=style, clickableFlag=True)
        t_color = "ForegroundWhite" if i == active_idx else "TextPrimary"
        
        # User's custom coordinates
        ic_x = 4 if i == 0 else 7
        ic_y = 5
        txt_x = 44
        txt_y = 11
        
        icon_lbl = base_widget("LVGLLabelWidget", ic_x, ic_y, -1, -1, use_style="LabelDefault", text=ic, textType="literal", text_font="fa24", text_color=t_color, recolor=False, useStaticText=True)
        text_lbl = base_widget("LVGLLabelWidget", txt_x, txt_y, -1, -1, use_style="LabelDefault", text=l, textType="literal", text_font="rm14", text_color=t_color, longMode="WRAP", recolor=False, useStaticText=True)
        
        btn["children"].extend([icon_lbl, text_lbl])
        btn["eventHandlers"] = [{"objID": gen_id(), "eventName": "CLICKED", "handlerType": "action", "action": f"load_{p}", "userData": 0}]
        sidebar["children"].append(btn)
        
    # Logout button
    btn_logout = base_widget("LVGLButtonWidget", 5, 270, 110, 40, use_style="ButtonDanger", clickableFlag=True)
    ic_out_lbl = base_widget("LVGLLabelWidget", 10, 8, -1, -1, use_style="LabelDefault", text="\uf015", textType="literal", text_font="fa24", text_color="ForegroundWhite", recolor=False, useStaticText=True)
    text_out_lbl = base_widget("LVGLLabelWidget", 40, 12, -1, -1, use_style="LabelDefault", text="Keluar", textType="literal", text_font="rm14", text_color="ForegroundWhite", longMode="WRAP", recolor=False, useStaticText=True)
    btn_logout["children"].extend([ic_out_lbl, text_out_lbl])
    btn_logout["eventHandlers"] = [{"objID": gen_id(), "eventName": "CLICKED", "handlerType": "action", "action": "load_Main", "userData": 0}]
    sidebar["children"].append(btn_logout)
    
    return sidebar

data = json.load(open('3inch5-display-rfid-barcode-gui.eez-project', encoding='utf-8'))

# Clean out broken pages
valid_pages = []
main_template = None
for p in data.get('userPages', []):
    if p['name'] in ['Admin_Panel', 'Admin_Password', 'Admin_Users', 'Admin_History', 'Admin_Settings', 'Admin_Test']:
        continue
    valid_pages.append(p)
    if p['name'] == 'main_screen':
        main_template = copy.deepcopy(p)

data['userPages'] = valid_pages

def create_page_from_template(name, components):
    new_page = copy.deepcopy(main_template)
    new_page['name'] = name
    new_page['objID'] = gen_id()
    new_page['components'][0]['objID'] = gen_id()
    # Reset local variables/events if any copied from Main
    new_page['localVariables'] = []
    # Replace children with our new components
    new_page['components'][0]['children'] = components
    return new_page

# 1. Admin Password
pwd_card = base_widget("LVGLPanelWidget", 60, 20, 360, 280, use_style="Card")
pwd_lbl = base_widget("LVGLLabelWidget", 0, 10, 360, -1, use_style="LabelDefault", text="Masukkan PIN Admin", textType="literal", text_align="CENTER", text_font="rm26", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)

pwd_input = base_widget("LVGLTextareaWidget", 60, 50, 240, 44, use_style="TextareaDefault", identifier="textarea_input_password")
pwd_input["passwordMode"] = True
pwd_input["placeholder"] = "PIN"
pwd_input["oneLineMode"] = True
pwd_input["text_align"] = "CENTER"

keypad = base_widget("LVGLPanelWidget", 60, 104, 240, 160, use_style="PanelTransparent")
keys = ["1", "2", "3", "4", "5", "6", "7", "8", "9", "Del", "0", "OK"]
kw, kh = 70, 35
for i, k in enumerate(keys):
    row, col = i // 3, i % 3
    btn = base_widget("LVGLButtonWidget", col*(kw+10), row*(kh+5), kw, kh, use_style="ButtonDefault", clickableFlag=True)
    btn["children"].append(base_widget("LVGLLabelWidget", 0, 0, -1, -1, use_style="LabelDefault", text=k, textType="literal", align="CENTER", text_font="rm14", longMode="WRAP", recolor=False, useStaticText=True))
    btn["eventHandlers"] = [{"objID": gen_id(), "eventName": "CLICKED", "handlerType": "action", "action": f"pwd_btn_{k}_pressed", "userData": 0}]
    keypad["children"].append(btn)

pwd_card["children"].extend([pwd_lbl, pwd_input, keypad])
data['userPages'].append(create_page_from_template("Admin_Password", [pwd_card]))

# 2. Admin_Users
u_card = base_widget("LVGLPanelWidget", 130, 10, 340, 300, use_style="Card")
u_title = base_widget("LVGLLabelWidget", 10, 10, 320, -1, use_style="LabelDefault", text="Manajemen Pengguna", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
btn_add = base_widget("LVGLButtonWidget", 10, 40, 90, 36, use_style="ButtonPrimary", clickableFlag=True, identifier="btn_users_add")
btn_add["children"].append(base_widget("LVGLLabelWidget", 0, 0, -1, -1, use_style="LabelDefault", text="Tambah", textType="literal", align="CENTER", text_font="rm14", text_color="ForegroundWhite", longMode="WRAP", recolor=False, useStaticText=True))
btn_edit = base_widget("LVGLButtonWidget", 110, 40, 90, 36, use_style="ButtonDefault", clickableFlag=True, identifier="btn_users_edit")
btn_edit["children"].append(base_widget("LVGLLabelWidget", 0, 0, -1, -1, use_style="LabelDefault", text="Edit", textType="literal", align="CENTER", text_font="rm14", longMode="WRAP", recolor=False, useStaticText=True))
btn_del = base_widget("LVGLButtonWidget", 210, 40, 90, 36, use_style="ButtonDanger", clickableFlag=True, identifier="btn_users_del")
btn_del["children"].append(base_widget("LVGLLabelWidget", 0, 0, -1, -1, use_style="LabelDefault", text="Hapus", textType="literal", align="CENTER", text_font="rm14", text_color="ForegroundWhite", longMode="WRAP", recolor=False, useStaticText=True))

# Table Header (y=90)
u_hdr = base_widget("LVGLPanelWidget", 10, 90, 320, 26, use_style="CardAccentSoft")
u_hdr_col1 = base_widget("LVGLLabelWidget", 5, 6, 80, -1, use_style="LabelDefault", text="UID", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
u_hdr_col2 = base_widget("LVGLLabelWidget", 90, 6, 120, -1, use_style="LabelDefault", text="Nama", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
u_hdr_col3 = base_widget("LVGLLabelWidget", 230, 6, 80, -1, use_style="LabelDefault", text="Tipe", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
u_hdr["children"].extend([u_hdr_col1, u_hdr_col2, u_hdr_col3])

u_line1 = base_widget("LVGLPanelWidget", 10, 116, 320, 1, use_style="CardAccent")

# Table Row 1 (y=124)
u_r1_col1 = base_widget("LVGLLabelWidget", 15, 124, 80, -1, use_style="LabelDefault", text="E2345", textType="literal", text_font="rm14", text_color="TextSecondary", longMode="WRAP", recolor=False, useStaticText=True, identifier="lbl_user1_uid")
u_r1_col2 = base_widget("LVGLLabelWidget", 100, 124, 120, -1, use_style="LabelDefault", text="Ahmad", textType="literal", text_font="rm14", text_color="TextSecondary", longMode="WRAP", recolor=False, useStaticText=True, identifier="lbl_user1_name")
u_r1_col3 = base_widget("LVGLLabelWidget", 240, 124, 80, -1, use_style="LabelDefault", text="RFID", textType="literal", text_font="rm14", text_color="TextSecondary", longMode="WRAP", recolor=False, useStaticText=True, identifier="lbl_user1_role")
u_line2 = base_widget("LVGLPanelWidget", 10, 144, 320, 1, use_style="CardAccent")

# Table Row 2 (y=152)
u_r2_col1 = base_widget("LVGLLabelWidget", 15, 152, 80, -1, use_style="LabelDefault", text="A1928", textType="literal", text_font="rm14", text_color="TextSecondary", longMode="WRAP", recolor=False, useStaticText=True, identifier="lbl_user2_uid")
u_r2_col2 = base_widget("LVGLLabelWidget", 100, 152, 120, -1, use_style="LabelDefault", text="Tamu", textType="literal", text_font="rm14", text_color="TextSecondary", longMode="WRAP", recolor=False, useStaticText=True, identifier="lbl_user2_name")
u_r2_col3 = base_widget("LVGLLabelWidget", 240, 152, 80, -1, use_style="LabelDefault", text="QR", textType="literal", text_font="rm14", text_color="TextSecondary", longMode="WRAP", recolor=False, useStaticText=True, identifier="lbl_user2_role")
u_line3 = base_widget("LVGLPanelWidget", 10, 172, 320, 1, use_style="CardAccent")

u_card["children"].extend([u_title, btn_add, btn_edit, btn_del, u_hdr, u_line1, u_r1_col1, u_r1_col2, u_r1_col3, u_line2, u_r2_col1, u_r2_col2, u_r2_col3, u_line3])
data['userPages'].append(create_page_from_template("Admin_Users", [make_sidebar(0), u_card]))

# 3. Admin_History
h_card = base_widget("LVGLPanelWidget", 130, 10, 340, 300, use_style="Card")
h_title = base_widget("LVGLLabelWidget", 10, 10, 320, -1, use_style="LabelDefault", text="Riwayat Akses", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)

# Table Header (y=40)
h_hdr = base_widget("LVGLPanelWidget", 10, 40, 320, 26, use_style="CardAccentSoft")
h_hdr_col1 = base_widget("LVGLLabelWidget", 5, 6, 80, -1, use_style="LabelDefault", text="Waktu", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
h_hdr_col2 = base_widget("LVGLLabelWidget", 90, 6, 120, -1, use_style="LabelDefault", text="Nama", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
h_hdr_col3 = base_widget("LVGLLabelWidget", 230, 6, 80, -1, use_style="LabelDefault", text="Metode", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
h_hdr["children"].extend([h_hdr_col1, h_hdr_col2, h_hdr_col3])

h_line1 = base_widget("LVGLPanelWidget", 10, 66, 320, 1, use_style="CardAccent")

# Table Row 1 (y=74)
h_r1_col1 = base_widget("LVGLLabelWidget", 15, 74, 80, -1, use_style="LabelDefault", text="10:05", textType="literal", text_font="rm14", text_color="TextSecondary", longMode="WRAP", recolor=False, useStaticText=True, identifier="lbl_hist1_time")
h_r1_col2 = base_widget("LVGLLabelWidget", 100, 74, 120, -1, use_style="LabelDefault", text="Ahmad", textType="literal", text_font="rm14", text_color="TextSecondary", longMode="WRAP", recolor=False, useStaticText=True, identifier="lbl_hist1_name")
h_r1_col3 = base_widget("LVGLLabelWidget", 240, 74, 80, -1, use_style="LabelDefault", text="RFID", textType="literal", text_font="rm14", text_color="TextSecondary", longMode="WRAP", recolor=False, useStaticText=True, identifier="lbl_hist1_method")
h_line2 = base_widget("LVGLPanelWidget", 10, 94, 320, 1, use_style="CardAccent")

# Table Row 2 (y=102)
h_r2_col1 = base_widget("LVGLLabelWidget", 15, 102, 80, -1, use_style="LabelDefault", text="11:30", textType="literal", text_font="rm14", text_color="TextSecondary", longMode="WRAP", recolor=False, useStaticText=True, identifier="lbl_hist2_time")
h_r2_col2 = base_widget("LVGLLabelWidget", 100, 102, 120, -1, use_style="LabelDefault", text="Tamu", textType="literal", text_font="rm14", text_color="TextSecondary", longMode="WRAP", recolor=False, useStaticText=True, identifier="lbl_hist2_name")
h_r2_col3 = base_widget("LVGLLabelWidget", 240, 102, 80, -1, use_style="LabelDefault", text="QR", textType="literal", text_font="rm14", text_color="TextSecondary", longMode="WRAP", recolor=False, useStaticText=True, identifier="lbl_hist2_method")
h_line3 = base_widget("LVGLPanelWidget", 10, 122, 320, 1, use_style="CardAccent")

h_card["children"].extend([h_title, h_hdr, h_line1, h_r1_col1, h_r1_col2, h_r1_col3, h_line2, h_r2_col1, h_r2_col2, h_r2_col3, h_line3])
data['userPages'].append(create_page_from_template("Admin_History", [make_sidebar(1), h_card]))

# 4. Admin_Settings
s_card = base_widget("LVGLPanelWidget", 130, 10, 340, 300, use_style="Card")
s_title = base_widget("LVGLLabelWidget", 10, 10, 320, -1, use_style="LabelDefault", text="Pengaturan Sistem", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)

# Left Column
lbl_ssid = base_widget("LVGLLabelWidget", 10, 40, 155, -1, use_style="LabelDefault", text="SSID WiFi:", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
inp_ssid = base_widget("LVGLTextareaWidget", 10, 60, 155, 36, use_style="TextareaDefault", identifier="inp_ssid", oneLineMode=True)

lbl_pass = base_widget("LVGLLabelWidget", 10, 106, 155, -1, use_style="LabelDefault", text="Pass WiFi:", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
inp_pass = base_widget("LVGLTextareaWidget", 10, 126, 155, 36, use_style="TextareaDefault", identifier="inp_pass", oneLineMode=True, passwordMode=True)

lbl_ip = base_widget("LVGLLabelWidget", 10, 172, 155, -1, use_style="LabelDefault", text="Static IP:", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
inp_ip = base_widget("LVGLTextareaWidget", 10, 192, 155, 36, use_style="TextareaDefault", identifier="inp_ip", oneLineMode=True)

# Right Column
lbl_ntp = base_widget("LVGLLabelWidget", 175, 40, 155, -1, use_style="LabelDefault", text="NTP Server:", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
inp_ntp = base_widget("LVGLTextareaWidget", 175, 60, 155, 36, use_style="TextareaDefault", identifier="inp_ntp", oneLineMode=True)

lbl_rel = base_widget("LVGLLabelWidget", 175, 106, 155, -1, use_style="LabelDefault", text="Durasi Pintu (detik):", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)
slider_rel = base_widget("LVGLSliderWidget", 175, 136, 155, 16, use_style="SliderDefault", identifier="slider_relay")
val_rel = base_widget("LVGLLabelWidget", 175, 162, 155, -1, use_style="LabelDefault", text="5 Detik", textType="literal", text_font="rm14", text_color="TextPrimary", identifier="lbl_relay_val", longMode="WRAP", recolor=False, useStaticText=True)

btn_save = base_widget("LVGLButtonWidget", 175, 192, 155, 36, use_style="ButtonPrimary", clickableFlag=True, identifier="btn_settings_save")
btn_save["children"].append(base_widget("LVGLLabelWidget", 0, 0, -1, -1, use_style="LabelDefault", text="Simpan", textType="literal", align="CENTER", text_font="rm14", text_color="ForegroundWhite", longMode="WRAP", recolor=False, useStaticText=True))

s_card["children"].extend([s_title, lbl_ssid, inp_ssid, lbl_pass, inp_pass, lbl_ip, inp_ip, lbl_ntp, inp_ntp, lbl_rel, slider_rel, val_rel, btn_save])
data['userPages'].append(create_page_from_template("Admin_Settings", [make_sidebar(2), s_card]))

# 5. Admin_Test
t_card = base_widget("LVGLPanelWidget", 130, 10, 340, 300, use_style="Card")
t_title = base_widget("LVGLLabelWidget", 10, 10, 320, -1, use_style="LabelDefault", text="Uji Perangkat Keras", textType="literal", text_font="rm14", text_color="TextPrimary", longMode="WRAP", recolor=False, useStaticText=True)

btn_t1 = base_widget("LVGLButtonWidget", 10, 50, 120, 44, use_style="ButtonDefault", clickableFlag=True, identifier="btn_test_relay")
btn_t1["children"].append(base_widget("LVGLLabelWidget", 0, 0, -1, -1, use_style="LabelDefault", text="Test Relay", textType="literal", align="CENTER", text_font="rm14", longMode="WRAP", recolor=False, useStaticText=True))
lbl_t1 = base_widget("LVGLLabelWidget", 140, 62, 190, -1, use_style="LabelDefault", text="Status: Menunggu", textType="literal", text_font="rm14", text_color="TextSecondary", identifier="lbl_test_relay", longMode="WRAP", recolor=False, useStaticText=True)

btn_t2 = base_widget("LVGLButtonWidget", 10, 110, 120, 44, use_style="ButtonDefault", clickableFlag=True, identifier="btn_test_camera")
btn_t2["children"].append(base_widget("LVGLLabelWidget", 0, 0, -1, -1, use_style="LabelDefault", text="Test Kamera", textType="literal", align="CENTER", text_font="rm14", longMode="WRAP", recolor=False, useStaticText=True))
lbl_t2 = base_widget("LVGLLabelWidget", 140, 122, 190, -1, use_style="LabelDefault", text="Status: Menunggu", textType="literal", text_font="rm14", text_color="TextSecondary", identifier="lbl_test_camera", longMode="WRAP", recolor=False, useStaticText=True)

btn_t3 = base_widget("LVGLButtonWidget", 10, 170, 120, 44, use_style="ButtonDefault", clickableFlag=True, identifier="btn_test_rfid")
btn_t3["children"].append(base_widget("LVGLLabelWidget", 0, 0, -1, -1, use_style="LabelDefault", text="Test RFID", textType="literal", align="CENTER", text_font="rm14", longMode="WRAP", recolor=False, useStaticText=True))
lbl_t3 = base_widget("LVGLLabelWidget", 140, 182, 190, -1, use_style="LabelDefault", text="Status: Menunggu", textType="literal", text_font="rm14", text_color="TextSecondary", identifier="lbl_test_rfid", longMode="WRAP", recolor=False, useStaticText=True)

t_card["children"].extend([t_title, btn_t1, lbl_t1, btn_t2, lbl_t2, btn_t3, lbl_t3])
data['userPages'].append(create_page_from_template("Admin_Test", [make_sidebar(3), t_card]))
# Fix bar and slider properties globally
def patch_widgets(comp):
    if comp.get('type') in ['LVGLBarWidget', 'LVGLSliderWidget']:
        if 'min' not in comp: comp['min'] = 0
        comp['minType'] = 'literal'
        if 'max' not in comp: comp['max'] = 100
        comp['maxType'] = 'literal'
        comp['mode'] = 'NORMAL'
        if 'value' not in comp: comp['value'] = 0
        comp['valueType'] = 'literal'
        comp['valueStart'] = 0
        comp['valueStartType'] = 'literal'
    for c in comp.get('children', []):
        patch_widgets(c)

for p in data.get('userPages', []):
    for c in p.get('components', []):
        patch_widgets(c)

# Fix missing actions
required_actions = [
    "pwd_btn_1_pressed", "pwd_btn_2_pressed", "pwd_btn_3_pressed",
    "pwd_btn_4_pressed", "pwd_btn_5_pressed", "pwd_btn_6_pressed",
    "pwd_btn_7_pressed", "pwd_btn_8_pressed", "pwd_btn_9_pressed",
    "pwd_btn_0_pressed", "pwd_btn_Del_pressed", "pwd_btn_OK_pressed",
    "load_Admin_Users", "load_Admin_History", "load_Admin_Settings",
    "load_Admin_Test", "load_Main"
]
if "actions" not in data:
    data["actions"] = []
existing_actions = [a["name"] for a in data["actions"]]
for a in required_actions:
    if a not in existing_actions:
        data["actions"].append({
            "objID": gen_id(),
            "name": a,
            "components": [],
            "connectionLines": [],
            "localVariables": [],
            "componentGroups": [],
            "userProperties": []
        })

json.dump(data, open('3inch5-display-rfid-barcode-gui.eez-project', 'w', encoding='utf-8'), indent=2)
print("Updated project using proper cloned templates")
