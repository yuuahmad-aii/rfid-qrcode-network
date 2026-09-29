import json
data = json.load(open('3inch5-display-rfid-barcode-gui.eez-project', encoding='utf-8'))

def find_widget(node, w_type):
    if node.get('type') == w_type:
        print(f"\n--- {w_type} ---")
        print({k:v for k,v in node.items() if k != 'children'})
        return True
    for c in node.get('children', []):
        if find_widget(c, w_type):
            return True
    return False

for p in data['userPages']:
    if p['name'] == 'Main':
        find_widget(p['components'][0], 'LVGLButtonWidget')
        find_widget(p['components'][0], 'LVGLTextareaWidget')
