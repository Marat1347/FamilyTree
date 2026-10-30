import json
import os

# Читаем данные из JSON
json_path = r"C:\python\Scripts\F_TREE\data\family_data.json"

with open(json_path, 'r', encoding='utf-8') as f:
    data = json.load(f)

# Форматируем JSON для вставки в HTML
json_str = json.dumps(data, ensure_ascii=False, indent=2)

html = f"""<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-capable" content="yes">
<title>Семейное дерево</title>
<script type="text/javascript" src="https://unpkg.com/vis-network/standalone/umd/vis-network.min.js"></script>
<style>
  html, body {{ height: 100%; margin: 0; background: #0D1420; color: #EAF0F8; font-family: 'Segoe UI', Arial, sans-serif; overflow: hidden; }}
  #network {{ width: 100%; height: 100%; }}
  .header {{ position: absolute; top: 0; left: 0; right: 0; background: rgba(20, 30, 46, 0.9); padding: 12px 16px; display: flex; justify-content: space-between; align-items: center; z-index: 10; pointer-events: none; box-shadow: 0 2px 10px rgba(0,0,0,0.5); }}
  .header h1 {{ margin: 0; font-size: 18px; font-weight: 600; }}
  .footer {{ position: absolute; bottom: 0; left: 0; right: 0; background: rgba(20, 30, 46, 0.9); padding: 8px 16px; font-size: 13px; color: #A3B4CC; z-index: 10; text-align: center; pointer-events: none; }}
</style>
</head>
<body>
<div class="header"><h1>Семейное дерево</h1><span id="stats"></span></div>
<div id="network"></div>
<div class="footer">Масштаб — двумя пальцами · Обзор — перетаскивание</div>

<script>
const familyData = {json_str};

const nodes = familyData.people.map(p => ({{
  id: p.id,
  label: p.name,
  level: p.generation,
  color: {{
    background: p.id === 'venera' ? '#E9BD5A' : (p.sex === 'F' ? '#ED9FC1' : '#6FB1F2'),
    border: '#1E2C42'
  }},
  font: {{ color: '#1B283B', bold: true, size: 14 }},
  shape: 'box',
  margin: 10,
  widthConstraint: 160
}}));

const edges = familyData.relations.map(r => ({{
  from: r.source,
  to: r.target,
  arrows: r.type === 'parent' ? 'to' : '',
  dashes: r.type === 'marriage',
  color: r.type === 'parent' ? '#677C99' : (r.type === 'marriage' ? '#ED9FC1' : '#859AB7')
}}));

const container = document.getElementById('network');
const data = {{ nodes: new vis.DataSet(nodes), edges: new vis.DataSet(edges) }};
const options = {{
  layout: {{
    hierarchical: {{
      direction: 'UD',
      sortMethod: 'directed',
      levelSeparation: 130,
      nodeSpacing: 200
    }}
  }},
  physics: false,
  interaction: {{ dragNodes: true, dragView: true, zoomView: true }}
}};
const network = new vis.Network(container, data, options);

document.getElementById('stats').textContent = familyData.people.length + ' персон';
</script>
</body>
</html>
"""

# Создаем папку mobile
os.makedirs(r"C:\python\Scripts\F_TREE\mobile", exist_ok=True)

out_path = r"C:\python\Scripts\F_TREE\mobile\family_tree.html"
with open(out_path, 'w', encoding='utf-8') as f:
    f.write(html)

print(f"✅ Мобильное приложение создано: {out_path}")