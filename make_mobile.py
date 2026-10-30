import json
import os

json_path = r"C:\python\Scripts\F_TREE\data\family_data.json"
with open(json_path, 'r', encoding='utf-8') as f:
    data = json.load(f)

json_str = json.dumps(data, ensure_ascii=False, indent=2)

html = f"""<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-capable" content="yes">
<title>Семейное дерево</title>
<link rel="manifest" href="manifest.json">
<script type="text/javascript" src="https://unpkg.com/vis-network/standalone/umd/vis-network.min.js"></script>
<style>
  :root {{
    --bg: #0D1420;
    --panel: #172233;
    --text: #EAF0F8;
    --muted: #A3B4CC;
    --male: #6FB1F2;
    --female: #ED9FC1;
    --gold: #E9BD5A;
  }}
  * {{ box-sizing: border-box; margin: 0; padding: 0; }}
  html, body {{ height: 100%; background: var(--bg); color: var(--text); font-family: 'Segoe UI', Arial, sans-serif; overflow: hidden; }}
  
  #network {{ width: 100%; height: 100%; }}
  
  /* Верхняя панель */
  .header {{
    position: absolute; top: 0; left: 0; right: 0;
    background: rgba(23, 34, 51, 0.95);
    padding: 12px 16px; display: flex; flex-direction: column; gap: 10px; z-index: 10;
    box-shadow: 0 4px 15px rgba(0,0,0,0.5);
    border-bottom-left-radius: 16px; border-bottom-right-radius: 16px;
  }}
  .header h1 {{ font-size: 18px; font-weight: 600; display: flex; justify-content: space-between; align-items: center; }}
  .search-box {{
    width: 100%; padding: 10px 14px; border-radius: 10px; border: none;
    background: rgba(255,255,255,0.1); color: white; font-size: 15px; outline: none;
  }}
  .search-box::placeholder {{ color: #8892a0; }}
  .filters {{ display: flex; gap: 8px; overflow-x: auto; padding-bottom: 4px; scrollbar-width: none; }}
  .filters::-webkit-scrollbar {{ display: none; }}
  .filter-btn {{
    background: rgba(255,255,255,0.1); border: none; color: var(--muted);
    padding: 6px 14px; border-radius: 20px; font-size: 13px; white-space: nowrap; cursor: pointer;
  }}
  .filter-btn.active {{ background: var(--male); color: #0D1420; font-weight: 600; }}

  /* Нижняя панель (Карточка) */
  .bottom-panel {{
    position: absolute; bottom: 0; left: 0; right: 0;
    background: rgba(23, 34, 51, 0.98);
    border-top-left-radius: 24px; border-top-right-radius: 24px;
    padding: 20px; padding-bottom: 40px;
    transform: translateY(100%); transition: transform 0.3s cubic-bezier(0.175, 0.885, 0.32, 1.275);
    z-index: 20; max-height: 75vh; display: flex; flex-direction: column;
    box-shadow: 0 -4px 20px rgba(0,0,0,0.5);
  }}
  .bottom-panel.open {{ transform: translateY(0); }}
  .panel-handle {{ width: 40px; height: 5px; background: #4a5b75; border-radius: 3px; margin: 0 auto 15px auto; }}
  .close-panel {{ position: absolute; top: 15px; right: 20px; background: none; border: none; color: var(--muted); font-size: 24px; cursor: pointer; }}
  
  .person-header {{ border-bottom: 1px solid rgba(255,255,255,0.1); padding-bottom: 12px; margin-bottom: 12px; }}
  .person-name {{ font-size: 22px; font-weight: bold; margin-bottom: 4px; }}
  .person-meta {{ font-size: 13px; color: var(--muted); }}
  
  .scroll-content {{ overflow-y: auto; flex: 1; padding-right: 5px; }}
  .rel-section {{ margin-top: 12px; }}
  .rel-title {{ font-size: 11px; text-transform: uppercase; color: var(--gold); margin-bottom: 6px; font-weight: bold; letter-spacing: 0.5px; }}
  .rel-list {{ display: flex; flex-direction: column; gap: 6px; }}
  .rel-item {{
    background: rgba(255,255,255,0.05); padding: 10px 12px; border-radius: 8px;
    font-size: 14px; cursor: pointer; display: flex; justify-content: space-between;
  }}
  .rel-item:active {{ background: rgba(255,255,255,0.15); }}
</style>
</head>
<body>
<div id="network"></div>

<div class="header" id="header">
  <h1><span>Семейное дерево</span><span id="stat" style="font-size:12px;color:var(--muted);font-weight:normal"></span></h1>
  <input type="text" id="search" class="search-box" placeholder="🔍 Поиск родственника...">
  <div class="filters">
    <button class="filter-btn active" data-branch="all">Все ветви</button>
    <button class="filter-btn" data-branch="paternal">Отцовская</button>
    <button class="filter-btn" data-branch="maternal">Материнская</button>
    <button class="filter-btn" data-branch="center">Центр</button>
  </div>
</div>

<div class="bottom-panel" id="panel">
  <div class="panel-handle"></div>
  <button class="close-panel" onclick="closePanel()">✕</button>
  <div class="person-header">
    <div class="person-name" id="p-name"></div>
    <div class="person-meta" id="p-meta"></div>
  </div>
  <div class="scroll-content" id="p-relatives"></div>
</div>

<script>
const familyData = {json_str};
let network = null;
let nodesDS = new vis.DataSet([]);
let edgesDS = new vis.DataSet([]);
let currentBranch = 'all';
let selectedId = null;

// Инициализация
const container = document.getElementById('network');
const data = {{ nodes: nodesDS, edges: edgesDS }};
const options = {{
  layout: {{
    hierarchical: {{
      direction: 'UD',
      sortMethod: 'directed',
      levelSeparation: 140,
      nodeSpacing: 150,
      treeSpacing: 200,
      blockShifting: true,
      edgeMinimization: true
    }}
  }},
  physics: false,
  interaction: {{
    dragNodes: false,
    dragView: true,
    zoomView: true,
    tooltipDelay: 300
  }},
  nodes: {{
    shape: 'box',
    margin: 12,
    font: {{ color: '#1B283B', bold: true, size: 14 }},
    borderWidth: 0,
    shadow: {{ enabled: true, color: 'rgba(0,0,0,0.5)', size: 5, x: 0, y: 2 }}
  }},
  edges: {{
    smooth: {{ type: 'cubicBezier', forceDirection: 'vertical', roundness: 0.4 }},
    width: 1.5
  }}
}};

network = new vis.Network(container, data, options);

// Отрисовка графа
function renderGraph() {{
  nodesDS.clear(); edgesDS.clear();
  
  const query = document.getElementById('search').value.toLowerCase().trim();
  const visibleNodes = new Set();
  
  // Фильтрация узлов
  familyData.people.forEach(p => {{
    let show = true;
    if (currentBranch !== 'all' && p.branch !== currentBranch) show = false;
    if (query && !p.name.toLowerCase().includes(query)) show = false;
    
    if (show) {{
      visibleNodes.add(p.id);
      let color = p.id === 'venera' ? '#E9BD5A' : (p.sex === 'F' ? '#ED9FC1' : '#6FB1F2');
      let label = p.name + (p.id === 'venera' ? '\\n★' : '');
      
      nodesDS.add({{
        id: p.id, label: label, level: p.generation,
        color: {{ background: color, border: color }},
        font: {{ color: '#1B283B', bold: true, size: 13 }},
        widthConstraint: {{ maximum: 160 }}
      }});
    }}
  }});

  // Фильтрация связей
  familyData.relations.forEach(r => {{
    if (visibleNodes.has(r.source) && visibleNodes.has(r.target)) {{
      let isMarriage = r.type === 'marriage';
      edgesDS.add({{
        from: r.source, to: r.target,
        arrows: r.type === 'parent' ? 'to' : '',
        dashes: isMarriage,
        color: r.type === 'parent' ? '#677C99' : (isMarriage ? '#ED9FC1' : '#859AB7'),
        width: isMarriage ? 2 : 1.5
      }});
    }}
  }});
  
  document.getElementById('stat').textContent = nodesDS.length + ' персон';
}}

// Клик по узлу
network.on("click", params => {{
  if (params.nodes.length > 0) {{
    showPanel(params.nodes[0]);
  }} else {{
    closePanel();
  }}
}});

// Показ нижней панели
function showPanel(id) {{
  selectedId = id;
  const p = familyData.people.find(x => x.id === id);
  if (!p) return;
  
  document.getElementById('p-name').textContent = p.name;
  document.getElementById('p-name').style.color = p.id === 'venera' ? '#E9BD5A' : (p.sex === 'F' ? '#ED9FC1' : '#6FB1F2');
  
  let meta = [];
  if (p.dates) meta.push(p.dates);
  if (p.branch) meta.push(p.branch === 'paternal' ? 'Отцовская' : p.branch === 'maternal' ? 'Материнская' : 'Центр');
  document.getElementById('p-meta').textContent = meta.join(' · ');

  // Поиск родственников
  let parents = [], spouses = [], children = [], siblings = [];
  familyData.relations.forEach(r => {{
    if (r.type === 'parent') {{
      if (r.target === id) parents.push(r.source);
      if (r.source === id) children.push(r.target);
    }} else if (r.source === id || r.target === id) {{
      let other = r.source === id ? r.target : r.source;
      if (r.type === 'marriage') spouses.push(other);
      else if (r.type === 'sibling') siblings.push(other);
    }}
  }});

  let html = '';
  function addBlock(title, arr) {{
    if (arr.length === 0) return;
    html += `<div class="rel-section"><div class="rel-title">${{title}} · ${{arr.length}}</div><div class="rel-list">`;
    arr.forEach(rid => {{
      let rp = familyData.people.find(x => x.id === rid);
      if (rp) html += `<div class="rel-item" onclick="focusPerson('${{rid}}')">${{rp.name}}</div>`;
    }});
    html += `</div></div>`;
  }}

  addBlock('Родители', parents);
  addBlock('Супруг(а)', spouses);
  addBlock('Дети', children);
  addBlock('Братья / Сёстры', siblings);
  
  if (!html) html = '<div style="color:#A3B4CC;font-size:14px;">Нет данных о родственниках</div>';
  document.getElementById('p-relatives').innerHTML = html;
  
  document.getElementById('panel').classList.add('open');
}}

function closePanel() {{
  document.getElementById('panel').classList.remove('open');
  selectedId = null;
}}

function focusPerson(id) {{
  network.focus(id, {{ scale: 1.0, animation: {{ duration: 500, easingFunction: 'easeInOutQuad' }} }});
  network.selectNodes([id]);
  showPanel(id);
}}

// События
document.getElementById('search').addEventListener('input', renderGraph);

document.querySelectorAll('.filter-btn').forEach(btn => {{
  btn.addEventListener('click', (e) => {{
    document.querySelectorAll('.filter-btn').forEach(b => b.classList.remove('active'));
    e.target.classList.add('active');
    currentBranch = e.target.getAttribute('data-branch');
    renderGraph();
    network.fit({{ animation: true }});
  }});
}});

// Запуск
renderGraph();
setTimeout(() => network.fit({{ animation: true }}), 100);
</script>
</body>
</html>
"""

os.makedirs(r"C:\python\Scripts\F_TREE\mobile", exist_ok=True)
out_path = r"C:\python\Scripts\F_TREE\mobile\family_tree.html"
with open(out_path, 'w', encoding='utf-8') as f:
    f.write(html)

print(f"✅ Мобильное приложение обновлено: {out_path}")