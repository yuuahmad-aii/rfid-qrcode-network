import json
data = json.load(open('3inch5-display-rfid-barcode-gui.eez-project', encoding='utf-8'))
for p in data['userPages']:
    if p['name'] == 'Main':
        # Find a label in Main
        for c in p['components'][0]['children']:
            if c.get('type') == 'LVGLPanelWidget':
                for c2 in c.get('children', []):
                    if c2.get('type') == 'LVGLLabelWidget':
                        print({k:v for k,v in c2.items() if k != 'children'})
                        exit(0)
