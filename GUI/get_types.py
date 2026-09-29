import json
data = json.load(open('TrailCurrentFireside.eez-project', encoding='utf-8'))
types = set()
def get_all(node):
    types.add(node.get('type'))
    for c in node.get('children', []):
        get_all(c)
for p in data.get('userPages', []):
    for c in p.get('components', []):
        get_all(c)
print(types)
