import json

data = json.load(open('TrailCurrentFireside.eez-project', encoding='utf-8'))

labels = []
def find_labels(w):
    if w.get('type') == 'LVGLLabelWidget':
        labels.append(w)
    for c in w.get('children', []):
        find_labels(c)

for p in data.get('userPages', []):
    for c in p.get('components', []):
        find_labels(c)
        
fa_labels = [l for l in labels if l.get('localStyles', {}).get('definition', {}).get('MAIN', {}).get('DEFAULT', {}).get('text_font', '').startswith('fa')]
unique_chars = set()
for l in fa_labels:
    text = l.get('text', '')
    for c in text:
        unique_chars.add(repr(c))
print("Unique FA characters used in reference project:")
print(", ".join(sorted(unique_chars)))
