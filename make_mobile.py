import json
import os
import urllib.request

JSON_PATH = r"C:\python\Scripts\F_TREE\data\family_data.json"
MOBILE_DIR = r"C:\python\Scripts\F_TREE\mobile"
VENDOR_DIR = os.path.join(MOBILE_DIR, "vendor")
VIS_PATH = os.path.join(VENDOR_DIR, "vis-network.min.js")
VIS_URL = "https://unpkg.com/vis-network@9.1.9/standalone/umd/vis-network.min.js"

os.makedirs(VENDOR_DIR, exist_ok=True)

if not os.path.exists(VIS_PATH):
    print("Скачиваю vis-network локально...")
    urllib.request.urlretrieve(VIS_URL, VIS_PATH)
    print(f"✅ Библиотека сохранена: {VIS_PATH}")
else:
    print(f"✅ Библиотека уже есть: {VIS_PATH}")

with open(JSON_PATH, 'r', encoding='utf-8') as f:
    data = json.load(f)
json_str = json.dumps(data, ensure_ascii=False, indent=2)

html = r"""<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="theme-color" content="#0D1420" id="meta-theme-color">
<link rel="manifest" href="manifest.json">
<title>Семейное дерево</title>
<script src="./vendor/vis-network.min.js"></script>
<style>
  :root {
    --bg: #0D1420;
    --panel: #172233;
    --surface: rgba(23, 34, 51, 0.95);
    --text: #EAF0F8;
    --muted: #A3B4CC;
    --male: #6FB1F2;
    --female: #ED9FC1;
    --gold: #E9BD5A;
    --path: #34D399;
    --card-top: #243349;
    --card-bottom: #1B283B;
    --border: rgba(255,255,255,0.1);
  }
  body.light-theme {
    --bg: #F5F2EB;
    --panel: #EAE4D7;
    --surface: rgba(234, 228, 215, 0.95);
    --text: #1E293B;
    --muted: #64748B;
    --male: #2563EB;
    --female: #DB2777;
    --gold: #B46E0C;
    --path: #059669;
    --card-top: #FFFFFF;
    --card-bottom: #F0ECE1;
    --border: rgba(0,0,0,0.1);
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  html, body { height: 100%; background: var(--bg); color: var(--text);
    font-family: 'Segoe UI', Arial, sans-serif; overflow: hidden;
    transition: background 0.3s, color 0.3s; }
  #network { width: 100%; height: 100%; }

  .header {
    position: absolute; top: 0; left: 0; right: 0;
    background: var(--surface); padding: 12px 16px;
    display: flex; flex-direction: column; gap: 10px; z-index: 10;
    box-shadow: 0 4px 15px rgba(0,0,0,0.2);
    border-bottom-left-radius: 16px; border-bottom-right-radius: 16px;
    backdrop-filter: blur(10px); -webkit-backdrop-filter: blur(10px);
  }
  .header h1 { font-size: 18px; font-weight: 600;
    display: flex; justify-content: space-between; align-items: center; }
  .search-box {
    width: 100%; padding: 10px 14px; border-radius: 10px; border: none;
    background: rgba(148,163,184,0.15); color: var(--text); font-size: 15px;
    outline: none; min-height: 44px;
  }
  .filters { display: flex; gap: 8px; overflow-x: auto; padding-bottom: 4px; scrollbar-width: none; }
  .filters::-webkit-scrollbar { display: none; }
  .filter-btn {
    background: rgba(148,163,184,0.15); border: none; color: var(--muted);
    padding: 8px 14px; border-radius: 20px; font-size: 13px;
    white-space: nowrap; cursor: pointer; min-height: 40px;
  }
  .filter-btn.active { background: var(--male); color: #FFF; font-weight: 600; }

  .path-banner {
    position: absolute; top: 138px; left: 16px; right: 16px;
    background: var(--path); color: #06281E;
    padding: 10px 16px; border-radius: 12px; font-size: 13px; font-weight: 600;
    display: none; justify-content: space-between; align-items: center;
    z-index: 15; box-shadow: 0 4px 16px rgba(0,0,0,0.35);
    backdrop-filter: blur(8px);
  }
  .path-banner.active { display: flex; }
  .path-banner button {
    background: rgba(0,0,0,0.18); border: none; color: #FFF;
    padding: 6px 12px; border-radius: 8px; font-size: 12px; cursor: pointer;
  }

  .bottom-panel {
    position: absolute; bottom: 0; left: 0; right: 0;
    background: var(--surface);
    border-top-left-radius: 24px; border-top-right-radius: 24px;
    padding: 20px; padding-bottom: 40px;
    transform: translateY(100%); transition: transform 0.3s cubic-bezier(0.175, 0.885, 0.32, 1.275);
    z-index: 20; max-height: 75vh; display: flex; flex-direction: column;
    box-shadow: 0 -4px 20px rgba(0,0,0,0.3);
    backdrop-filter: blur(10px); -webkit-backdrop-filter: blur(10px);
  }
  .bottom-panel.open { transform: translateY(0); }
  .panel-handle { width: 40px; height: 5px; background: var(--muted);
    border-radius: 3px; margin: 0 auto 15px auto; opacity: 0.5; }
  .close-panel { position: absolute; top: 15px; right: 20px;
    background: none; border: none; color: var(--muted); font-size: 24px; cursor: pointer; }
  .person-header { border-bottom: 1px solid var(--border);
    padding-bottom: 12px; margin-bottom: 12px; }
  .person-name { font-size: 22px; font-weight: bold; margin-bottom: 4px; }
  .person-meta { font-size: 13px; color: var(--muted); }
  
  .person-actions { display: flex; gap: 8px; margin-top: 10px; }
  .action-chip {
    flex: 1; background: rgba(148,163,184,0.15); border: 1px solid var(--border);
    color: var(--text); padding: 10px; border-radius: 10px;
    font-size: 12px; font-weight: 600; cursor: pointer; text-align: center;
  }
  .action-chip.gold { border-color: var(--gold); color: var(--gold); }

  .scroll-content { overflow-y: auto; flex: 1; padding-right: 5px; }
  .rel-section { margin-top: 12px; }
  .rel-title { font-size: 11px; text-transform: uppercase;
    color: var(--gold); margin-bottom: 6px; font-weight: bold; letter-spacing: 0.5px; }
  .rel-list { display: flex; flex-direction: column; gap: 6px; }
  .rel-item { background: rgba(148,163,184,0.1); padding: 12px;
    border-radius: 8px; font-size: 14px; cursor: pointer;
    display: flex; justify-content: space-between; min-height: 44px; align-items: center; }
  .rel-item:active { background: rgba(148,163,184,0.25); }

  .fab-dock {
    position: absolute; right: 16px; bottom: 24px;
    display: flex; flex-direction: column; gap: 10px; z-index: 12;
  }
  .fab-btn {
    width: 48px; height: 48px; border-radius: 14px;
    background: var(--surface);
    border: 1px solid var(--border);
    color: var(--text); font-size: 20px; cursor: pointer;
    display: flex; align-items: center; justify-content: center;
    box-shadow: 0 4px 14px rgba(0,0,0,0.2);
    backdrop-filter: blur(10px);
  }
  .fab-btn:active { transform: scale(0.93); }

  .toast {
    position: fixed; bottom: 80px; left: 50%;
    transform: translateX(-50%) translateY(100px);
    background: var(--panel); color: var(--text);
    padding: 12px 20px; border-radius: 12px; font-size: 14px;
    box-shadow: 0 4px 20px rgba(0,0,0,0.4); z-index: 100;
    opacity: 0; transition: all 0.3s; pointer-events: none;
    max-width: 90vw; text-align: center;
  }
  .toast.show { opacity: 1; transform: translateX(-50%) translateY(0); }
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

<div class="path-banner" id="path-banner">
  <span id="path-text">Маршрут родства</span>
  <button onclick="clearPath()">Сбросить ✕</button>
</div>

<div class="fab-dock">
  <button class="fab-btn" title="К Венере" onclick="focusPerson('venera')">★</button>
  <button class="fab-btn" title="Показать всё" onclick="network.fit({animation:true})">⛶</button>
  <button class="fab-btn" id="theme-btn" title="Сменить тему" onclick="toggleTheme()">☀️</button>
  <button class="fab-btn" title="Экспорт PNG" onclick="exportPNG()">📷</button>
</div>

<div class="bottom-panel" id="panel">
  <div class="panel-handle"></div>
  <button class="close-panel" onclick="closePanel()">✕</button>
  <div class="person-header">
    <div class="person-name" id="p-name"></div>
    <div class="person-meta" id="p-meta"></div>
    <div class="person-actions" id="p-actions"></div>
  </div>
  <div class="scroll-content" id="p-relatives"></div>
</div>

<div class="toast" id="toast"></div>

<script>
const familyData = __JSON_DATA__;

let network = null;
let nodesDS = new vis.DataSet([]);
let edgesDS = new vis.DataSet([]);
let currentBranch = 'all';
let selectedId = null;
let isLightTheme = false;
let pathNodes = new Set();
let pathEdges = new Set();
let pathPickSourceId = null; // Режим связывания

const palettes = {
  dark: {
    bg: '#0D1420', panel: '#172233', surface: 'rgba(23,34,51,0.95)',
    text: '#EAF0F8', muted: '#A3B4CC', male: '#6FB1F2', female: '#ED9FC1',
    gold: '#E9BD5A', path: '#34D399', cardTop: '#243349', cardBottom: '#1B283B',
    border: 'rgba(255,255,255,0.1)'
  },
  light: {
    bg: '#F5F2EB', panel: '#EAE4D7', surface: 'rgba(234,228,215,0.95)',
    text: '#1E293B', muted: '#64748B', male: '#2563EB', female: '#DB2777',
    gold: '#B46E0C', path: '#059669', cardTop: '#FFFFFF', cardBottom: '#F0ECE1',
    border: 'rgba(0,0,0,0.1)'
  }
};
let currentPalette = palettes.dark;

const container = document.getElementById('network');
const options = {
  layout: {
    hierarchical: {
      direction: 'UD', sortMethod: 'directed',
      levelSeparation: 155, nodeSpacing: 175, treeSpacing: 210,
      blockShifting: true, edgeMinimization: true
    }
  },
  physics: false,
  interaction: { dragNodes: false, dragView: true, zoomView: true, hover: false },
  nodes: {
    shape: 'custom', borderWidth: 2, borderWidthSelected: 3.5,
    font: { face: 'Segoe UI, Roboto, sans-serif', size: 14 },
    shadow: { enabled: true, color: 'rgba(0,0,0,0.35)', size: 8, x: 0, y: 4 }
  },
  edges: {
    smooth: { type: 'cubicBezier', forceDirection: 'vertical', roundness: 0.45 },
    width: 1.8
  }
};

// ============================================================
// УМНЫЕ КАРТОЧКИ (Canvas ctxRenderer)
// ============================================================
function wrapText(ctx, text, maxWidth) {
  const words = String(text).split(' ');
  const lines = [];
  let currentLine = '';
  for (const word of words) {
    const testLine = currentLine ? currentLine + ' ' + word : word;
    if (ctx.measureText(testLine).width > maxWidth && currentLine) {
      lines.push(currentLine);
      currentLine = word;
    } else {
      currentLine = testLine;
    }
  }
  if (currentLine) lines.push(currentLine);
  return lines;
}

function getNodeConfig(p, isExporting) {
  const scale = network ? network.getScale() : 1;
  const isSelected = selectedId === p.id;
  const inPath = pathNodes.has(p.id);
  const query = document.getElementById('search').value.toLowerCase().trim();
  const isSearchMatch = query && p.name.toLowerCase().includes(query);
  const isDimmed = (query && !isSearchMatch) || (pathNodes.size > 0 && !inPath);

  const accent = p.id === 'venera' ? currentPalette.gold
    : (p.sex === 'F' ? currentPalette.female : currentPalette.male);
  const borderCol = inPath ? currentPalette.path
    : (isSearchMatch ? '#F59E0B' : accent);
  const bgTop = p.id === 'venera' && !isExporting ? (isLightTheme ? '#FEF3C7' : '#2D271C') : currentPalette.cardTop;
  const bgBottom = p.id === 'venera' && !isExporting ? (isLightTheme ? '#FDE68A' : '#1F1A10') : currentPalette.cardBottom;

  return {
    id: p.id,
    level: p.generation,
    opacity: isDimmed ? 0.25 : 1.0,
    borderWidth: (inPath || isSearchMatch || p.id === 'venera') ? 3 : 2,
    color: { border: borderCol },
    ctxRenderer: function({ ctx, id, x, y, state }) {
      const isHov = state.hover;
      const w = isExporting ? 170 : (scale < 0.35 ? 60 : (scale > 0.65 ? 170 : 140));
      const padding = 10;
      
      // Определяем, что рисовать
      let titleLines = [];
      let subText = '';
      
      if (isExporting || isSelected || scale > 0.65) {
        // Полный формат
        ctx.font = 'bold 14px "Segoe UI", sans-serif';
        titleLines = wrapText(ctx, p.name, w - padding * 2);
        let subs = [];
        if (p.maiden) subs.push(`ур. ${p.maiden}`);
        if (p.dates) subs.push(p.dates);
        subText = subs.join(' · ');
      } else if (scale > 0.35) {
        // Только имя (макс 2 строки)
        ctx.font = 'bold 13px "Segoe UI", sans-serif';
        titleLines = wrapText(ctx, p.name, w - padding * 2);
      } else {
        // Только инициалы
        ctx.font = 'bold 16px "Segoe UI", sans-serif';
        const parts = p.name.split(' ');
        titleLines = [parts[0][0] + (parts[1] ? parts[1][0] : '')];
      }

      // Вычисляем высоту
      let h = padding * 2;
      h += titleLines.length * 18;
      if (subText) h += 16;
      if (p.id === 'venera') h += 6; // Место под бейдж

      // Рисуем скругленный прямоугольник с градиентом
      const r = 12;
      const left = x - w / 2, top = y - h / 2;
      
      const grad = ctx.createLinearGradient(0, top, 0, top + h);
      grad.addColorStop(0, bgTop);
      grad.addColorStop(1, bgBottom);

      ctx.beginPath();
      ctx.moveTo(left + r, top);
      ctx.lineTo(left + w - r, top);
      ctx.quadraticCurveTo(left + w, top, left + w, top + r);
      ctx.lineTo(left + w, top + h - r);
      ctx.quadraticCurveTo(left + w, top + h, left + w - r, top + h);
      ctx.lineTo(left + r, top + h);
      ctx.quadraticCurveTo(left, top + h, left, top + h - r);
      ctx.lineTo(left, top + r);
      ctx.quadraticCurveTo(left, top, left + r, top);
      ctx.closePath();

      ctx.fillStyle = grad;
      ctx.fill();
      ctx.lineWidth = isHov ? 4 : (inPath || isSelected || isSearchMatch ? 3 : 2);
      ctx.strokeStyle = borderCol;
      ctx.stroke();

      // Цветная точка акцента (слева)
      ctx.beginPath();
      ctx.arc(left + 12, top + 14, 4, 0, Math.PI * 2);
      ctx.fillStyle = accent;
      ctx.fill();

      // Текст
      ctx.textAlign = 'center';
      ctx.textBaseline = 'top';
      ctx.fillStyle = currentPalette.text;
      
      let textY = top + padding;
      titleLines.forEach((line, i) => {
        ctx.font = 'bold ' + (isExporting || isSelected || scale > 0.65 ? 14 : (scale > 0.35 ? 13 : 16)) + 'px "Segoe UI", sans-serif';
        ctx.fillText(line, x, textY);
        textY += 18;
      });

      if (subText) {
        ctx.font = '11px "Segoe UI", sans-serif';
        ctx.fillStyle = currentPalette.muted;
        ctx.fillText(subText, x, textY);
        textY += 16;
      }

      if (p.id === 'venera') {
        ctx.font = 'bold 10px "Segoe UI", sans-serif';
        ctx.fillStyle = currentPalette.gold;
        ctx.fillText('★ ЦЕНТР', x, textY);
      }

      return { drawExternalLabels: null };
    }
  };
}

function makeEdge(r, index) {
  const isMarriage = r.type === 'marriage';
  const inPath = pathEdges.has('e_' + index);
  let color = r.type === 'parent' ? '#677C99' : (isMarriage ? '#ED9FC1' : '#859AB7');
  if (inPath) color = currentPalette.path;
  else if (pathNodes.size > 0) color = 'rgba(148,163,184,0.15)';

  return {
    id: 'e_' + index,
    from: r.source,
    to: r.target,
    arrows: r.type === 'parent' ? { to: { enabled: true, scaleFactor: 0.65 } } : '',
    dashes: isMarriage && !inPath,
    color: color,
    width: inPath ? 4 : (isMarriage ? 2.2 : 1.5)
  };
}

function renderGraph() {
  nodesDS.clear();
  edgesDS.clear();

  const query = document.getElementById('search').value.toLowerCase().trim();
  const visibleNodes = new Set();

  familyData.people.forEach(p => {
    let show = true;
    if (currentBranch !== 'all' && p.branch !== currentBranch) show = false;
    if (query && !p.name.toLowerCase().includes(query)) show = false;
    if (show) {
      visibleNodes.add(p.id);
      nodesDS.add(getNodeConfig(p, false));
    }
  });

  familyData.relations.forEach((r, i) => {
    if (visibleNodes.has(r.source) && visibleNodes.has(r.target)) {
      edgesDS.add(makeEdge(r, i));
    }
  });

  document.getElementById('stat').textContent = nodesDS.length + ' персон';
}

network = new vis.Network(container, { nodes: nodesDS, edges: edgesDS }, options);

// ============================================================
// ПОИСК ПУТИ (BFS)
// ============================================================
function findShortestPath(startId, targetId) {
  if (startId === targetId) return null;
  const adj = {};
  familyData.relations.forEach(r => {
    if (!adj[r.source]) adj[r.source] = [];
    if (!adj[r.target]) adj[r.target] = [];
    adj[r.source].push({ to: r.target, id: 'e_' + familyData.relations.indexOf(r) });
    adj[r.target].push({ to: r.source, id: 'e_' + familyData.relations.indexOf(r) });
  });

  const prev = { [startId]: null };
  const queue = [startId];
  while (queue.length) {
    const curr = queue.shift();
    if (curr === targetId) break;
    for (const next of (adj[curr] || [])) {
      if (!(next.to in prev)) {
        prev[next.to] = { from: curr, edge: next.id };
        queue.push(next.to);
      }
    }
  }
  if (!(targetId in prev)) return null;

  const nodes = [targetId];
  const edges = [];
  let cur = targetId;
  while (cur !== startId) {
    const step = prev[cur];
    edges.push(step.edge);
    cur = step.from;
    nodes.push(cur);
  }
  return { nodes: nodes.reverse(), edges: edges.reverse() };
}

function applyPath(from, to) {
  const res = findShortestPath(from, to);
  const banner = document.getElementById('path-banner');
  if (!res) {
    document.getElementById('path-text').textContent = 'Связь не найдена';
    banner.classList.add('active');
    return;
  }
  pathNodes = new Set(res.nodes);
  pathEdges = new Set(res.edges);
  
  const p1 = familyData.people.find(x => x.id === from);
  const p2 = familyData.people.find(x => x.id === to);
  document.getElementById('path-text').textContent = `🔗 ${p1.name.split(' ')[0]} ↔ ${p2.name.split(' ')[0]}: ${res.nodes.length - 1} шаг(а)`;
  banner.classList.add('active');
  
  renderGraph();
  closePanel();
  network.fit({ nodes: res.nodes, animation: { duration: 600 } });
}

function startLink(id) {
  pathPickSourceId = id;
  closePanel();
  const p = familyData.people.find(x => x.id === id);
  document.getElementById('path-text').textContent = `👆 Нажмите на второго человека (от: ${p.name.split(' ')[0]})`;
  document.getElementById('path-banner').classList.add('active');
}

function clearPath() {
  pathNodes.clear();
  pathEdges.clear();
  pathPickSourceId = null;
  document.getElementById('path-banner').classList.remove('active');
  renderGraph();
}

// ============================================================
// ТЕМА
// ============================================================
function toggleTheme() {
  isLightTheme = !isLightTheme;
  document.body.classList.toggle('light-theme', isLightTheme);
  currentPalette = isLightTheme ? palettes.light : palettes.dark;
  document.getElementById('theme-btn').textContent = isLightTheme ? '🌙' : '☀️';
  document.getElementById('meta-theme-color').content = currentPalette.bg;
  try { localStorage.setItem('ftree-theme', isLightTheme ? 'light' : 'dark'); } catch(e){}
  renderGraph();
  if (selectedId) showPanel(selectedId);
}

// ============================================================
// ПАНЕЛЬ
// ============================================================
network.on('click', params => {
  if (params.nodes.length > 0) {
    const clicked = params.nodes[0];
    if (pathPickSourceId) {
      const src = pathPickSourceId;
      pathPickSourceId = null;
      applyPath(src, clicked);
      return;
    }
    showPanel(clicked);
  } else {
    closePanel();
  }
});

function showPanel(id) {
  selectedId = id;
  const p = familyData.people.find(x => x.id === id);
  if (!p) return;

  document.getElementById('p-name').textContent = p.name;
  document.getElementById('p-name').style.color = p.id === 'venera' ? currentPalette.gold
    : (p.sex === 'F' ? currentPalette.female : currentPalette.male);

  let meta = [];
  if (p.dates) meta.push(p.dates);
  if (p.branch) meta.push(p.branch === 'paternal' ? 'Отцовская' : p.branch === 'maternal' ? 'Материнская' : 'Центр');
  document.getElementById('p-meta').textContent = meta.join(' · ');

  let actions = '';
  if (id !== 'venera') actions += `<button class="action-chip gold" onclick="applyPath('${id}','venera')">★ Путь до Венеры</button>`;
  actions += `<button class="action-chip" onclick="startLink('${id}')">🧭 Связать с...</button>`;
  document.getElementById('p-actions').innerHTML = actions;

  let parents = [], spouses = [], children = [], siblings = [];
  familyData.relations.forEach(r => {
    if (r.type === 'parent') {
      if (r.target === id) parents.push(r.source);
      if (r.source === id) children.push(r.target);
    } else if (r.source === id || r.target === id) {
      let other = r.source === id ? r.target : r.source;
      if (r.type === 'marriage') spouses.push(other);
      else if (r.type === 'sibling') siblings.push(other);
    }
  });

  let html = '';
  function addBlock(title, arr) {
    if (arr.length === 0) return;
    html += '<div class="rel-section"><div class="rel-title">' + title + ' · ' + arr.length + '</div><div class="rel-list">';
    arr.forEach(rid => {
      let rp = familyData.people.find(x => x.id === rid);
      if (rp) html += '<div class="rel-item" onclick="focusPerson(\'' + rid + '\')">' + rp.name + '</div>';
    });
    html += '</div></div>';
  }
  addBlock('Родители', parents);
  addBlock('Супруг(а)', spouses);
  addBlock('Дети', children);
  addBlock('Братья / Сёстры', siblings);

  if (!html) html = '<div style="color:#A3B4CC;font-size:14px;">Нет данных о родственниках</div>';
  document.getElementById('p-relatives').innerHTML = html;
  document.getElementById('panel').classList.add('open');
}

function closePanel() {
  document.getElementById('panel').classList.remove('open');
  selectedId = null;
}

function focusPerson(id) {
  network.focus(id, { scale: 1.0, animation: { duration: 500, easingFunction: 'easeInOutQuad' } });
  network.selectNodes([id]);
  showPanel(id);
}

// ============================================================
// ЭКСПОРТ PNG
// ============================================================
function nextDraw(net) {
  return new Promise((resolve, reject) => {
    let timeout;
    net.once('afterDrawing', () => { clearTimeout(timeout); resolve(); });
    timeout = setTimeout(() => reject(new Error('Таймаут')), 6000);
    net.redraw();
  });
}

async function exportPNG() {
  toast('Готовлю PNG...', 1500);
  await document.fonts.ready;

  const host = document.createElement('div');
  const dpr = window.devicePixelRatio || 1;
  host.style.cssText = 'position:fixed;left:-100000px;top:0;width:' +
    Math.max(1, Math.floor(3600 / dpr)) + 'px;height:' +
    Math.max(1, Math.floor(2400 / dpr)) + 'px;pointer-events:none;';
  document.body.append(host);

  let temp = null;
  try {
    const allNodes = familyData.people.map(p => getNodeConfig(p, true));
    const allEdges = familyData.relations.map((r, i) => makeEdge(r, i));

    temp = new vis.Network(host, {
      nodes: new vis.DataSet(allNodes),
      edges: new vis.DataSet(allEdges)
    }, options);

    await nextDraw(temp);
    temp.fit({ animation: false });
    temp.moveTo({ scale: temp.getScale() * 0.85 });
    await nextDraw(temp);

    const srcCanvas = host.querySelector('canvas');
    if (!srcCanvas || !srcCanvas.width) throw new Error('Canvas не создан');
    if (srcCanvas.width * srcCanvas.height > 12000000) throw new Error('Слишком большой canvas');

    const outCanvas = document.createElement('canvas');
    outCanvas.width = srcCanvas.width;
    outCanvas.height = srcCanvas.height;
    const ctx = outCanvas.getContext('2d');
    if (!ctx) throw new Error('Нет памяти');

    ctx.fillStyle = currentPalette.bg;
    ctx.fillRect(0, 0, outCanvas.width, outCanvas.height);
    ctx.drawImage(srcCanvas, 0, 0);

    ctx.fillStyle = currentPalette.muted;
    ctx.font = 'bold 22px "Segoe UI", sans-serif';
    ctx.fillText('Семейное дерево · ' + familyData.people.length + ' персон', 28, outCanvas.height - 28);

    outCanvas.toBlob(async (blob) => {
      if (!blob) { toast('Не удалось создать PNG'); return; }
      const file = new File([blob], 'family_tree.png', { type: 'image/png' });
      if (navigator.canShare && navigator.canShare({ files: [file] })) {
        try {
          await navigator.share({ files: [file], title: 'Семейное дерево' });
          toast('Отправлено!'); return;
        } catch (e) { if (e.name === 'AbortError') return; }
      }
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url; a.download = 'family_tree.png';
      document.body.appendChild(a); a.click(); a.remove();
      URL.revokeObjectURL(url);
      toast('PNG сохранён');
    }, 'image/png');
  } catch (e) {
    toast('Ошибка: ' + e.message, 4000);
  } finally {
    if (temp) temp.destroy();
    host.remove();
  }
}

// ============================================================
// СОБЫТИЯ
// ============================================================
document.getElementById('search').addEventListener('input', renderGraph);
document.querySelectorAll('.filter-btn').forEach(btn => {
  btn.addEventListener('click', e => {
    document.querySelectorAll('.filter-btn').forEach(b => b.classList.remove('active'));
    e.target.classList.add('active');
    currentBranch = e.target.getAttribute('data-branch');
    renderGraph();
    network.fit({ animation: true });
  });
});

// Init theme
if (localStorage.getItem('ftree-theme') === 'light') {
  isLightTheme = true;
  document.body.classList.add('light-theme');
  currentPalette = palettes.light;
  document.getElementById('theme-btn').textContent = '🌙';
  document.getElementById('meta-theme-color').content = currentPalette.bg;
}

if ('serviceWorker' in navigator && location.protocol === 'https:') {
  window.addEventListener('load', () => {
    navigator.serviceWorker.register('./sw.js', { scope: './' }).catch(err => console.warn(err));
  });
}

renderGraph();
setTimeout(() => network.fit({ animation: true }), 100);
</script>
</body>
</html>
"""

html = html.replace("__JSON_DATA__", json_str)

os.makedirs(MOBILE_DIR, exist_ok=True)
out_path = os.path.join(MOBILE_DIR, "family_tree.html")
with open(out_path, 'w', encoding='utf-8') as f:
    f.write(html)
print(f"✅ family_tree.html создан: {out_path}")

sw = r"""const CACHE = 'family-tree-v2';
const ASSETS = ['./', './family_tree.html', './manifest.json', './vendor/vis-network.min.js'];

self.addEventListener('install', e => {
  self.skipWaiting();
  e.waitUntil(caches.open(CACHE).then(c => c.addAll(ASSETS)));
});

self.addEventListener('activate', e => {
  e.waitUntil(caches.keys().then(keys => Promise.all(
    keys.filter(k => k !== CACHE).map(k => caches.delete(k))
  )).then(() => self.clients.claim()));
});

self.addEventListener('fetch', e => {
  e.respondWith(caches.match(e.request).then(cached => {
    return cached || fetch(e.request).then(r => {
      if (r && r.status === 200) {
        const clone = r.clone();
        caches.open(CACHE).then(c => c.put(e.request, clone));
      }
      return r;
    });
  }));
});
"""

sw_path = os.path.join(MOBILE_DIR, "sw.js")
with open(sw_path, 'w', encoding='utf-8') as f:
    f.write(sw)
print(f"✅ sw.js создан: {sw_path}")

manifest = {
    "name": "Семейное дерево",
    "short_name": "Дерево",
    "start_url": "./family_tree.html",
    "display": "standalone",
    "background_color": "#0D1420",
    "theme_color": "#0D1420",
    "icons": [{"src": "icon.png", "sizes": "256x256", "type": "image/png"}]
}
manifest_path = os.path.join(MOBILE_DIR, "manifest.json")
with open(manifest_path, 'w', encoding='utf-8') as f:
    json.dump(manifest, f, ensure_ascii=False, indent=2)
print(f"✅ manifest.json создан: {manifest_path}")