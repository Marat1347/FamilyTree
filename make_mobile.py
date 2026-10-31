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
    urllib.request.urlretrieve(VIS_URL, VIS_PATH)

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
    --bg: #0D1420; --panel: #172233; --surface: rgba(23, 34, 51, 0.95);
    --text: #EAF0F8; --muted: #A3B4CC; --male: #6FB1F2; --female: #ED9FC1;
    --gold: #E9BD5A; --path: #34D399; --border: rgba(255,255,255,0.1);
  }
  body.light-theme {
    --bg: #F5F2EB; --panel: #EAE4D7; --surface: rgba(234, 228, 215, 0.95);
    --text: #1E293B; --muted: #64748B; --male: #8FB3D9; --female: #EBA7BE;
    --gold: #D4A359; --path: #059669; --border: rgba(0,0,0,0.1);
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  html, body { height: 100%; background: var(--bg); color: var(--text);
    font-family: 'Segoe UI', Arial, sans-serif; overflow: hidden;
    transition: background 0.2s, color 0.2s; }
  #network { width: 100%; height: 100%; }

  #spinner {
    position: absolute; top: 50%; left: 50%;
    transform: translate(-50%, -50%);
    text-align: center; z-index: 5;
  }
  #spinner .ring {
    width: 50px; height: 50px; margin: 0 auto 16px;
    border: 3px solid rgba(163,180,204,0.2);
    border-top-color: var(--male);
    border-radius: 50%;
    animation: spin 0.8s linear infinite;
  }
  @keyframes spin { to { transform: rotate(360deg); } }
  #spinner .text { color: var(--muted); font-size: 14px; }

  .header {
    position: absolute; top: 0; left: 0; right: 0;
    background: var(--surface); padding: 12px 16px;
    display: flex; flex-direction: column; gap: 10px; z-index: 10;
    box-shadow: 0 4px 15px rgba(0,0,0,0.2);
    border-bottom-left-radius: 16px; border-bottom-right-radius: 16px;
    backdrop-filter: blur(10px);
  }
  .header h1 { font-size: 18px; font-weight: 600;
    display: flex; justify-content: space-between; align-items: center; }
  .search-box {
    width: 100%; padding: 10px 14px; border-radius: 10px; border: none;
    background: rgba(148,163,184,0.15); color: var(--text); font-size: 15px;
    outline: none; min-height: 44px;
  }
  .filters { display: flex; gap: 8px; overflow-x: auto; padding-bottom: 4px; }
  .filter-btn {
    background: rgba(148,163,184,0.15); border: none; color: var(--muted);
    padding: 8px 14px; border-radius: 20px; font-size: 13px; min-height: 40px;
  }
  .filter-btn.active { background: var(--male); color: #FFF; font-weight: 600; }

  .path-banner {
    position: absolute; top: 138px; left: 16px; right: 16px;
    background: var(--path); color: #06281E;
    padding: 10px 16px; border-radius: 12px; font-size: 13px; font-weight: 600;
    display: none; justify-content: space-between; align-items: center;
    z-index: 15; box-shadow: 0 4px 16px rgba(0,0,0,0.35);
  }
  .path-banner.active { display: flex; }
  .path-banner button {
    background: rgba(0,0,0,0.18); border: none; color: #FFF;
    padding: 6px 12px; border-radius: 8px; font-size: 12px; cursor: pointer;
  }

  .bottom-panel {
    position: absolute; bottom: 0; left: 0; right: 0;
    background: var(--surface); border-top-left-radius: 24px; border-top-right-radius: 24px;
    padding: 20px; padding-bottom: 40px; transform: translateY(100%);
    transition: transform 0.25s cubic-bezier(0.175, 0.885, 0.32, 1.2);
    z-index: 20; max-height: 75vh; display: flex; flex-direction: column;
    box-shadow: 0 -4px 20px rgba(0,0,0,0.3);
  }
  .bottom-panel.open { transform: translateY(0); }
  .panel-handle { width: 40px; height: 5px; background: var(--muted);
    border-radius: 3px; margin: 0 auto 15px auto; opacity: 0.5; }
  .close-panel { position: absolute; top: 15px; right: 20px;
    background: none; border: none; color: var(--muted); font-size: 24px; }
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

  .scroll-content { overflow-y: auto; flex: 1; }
  .rel-section { margin-top: 12px; }
  .rel-title { font-size: 11px; text-transform: uppercase;
    color: var(--gold); margin-bottom: 6px; font-weight: bold; }
  .rel-list { display: flex; flex-direction: column; gap: 6px; }
  .rel-item { background: rgba(148,163,184,0.1); padding: 12px;
    border-radius: 8px; font-size: 14px; cursor: pointer;
    display: flex; justify-content: space-between; min-height: 44px; }
  .rel-item:active { background: rgba(148,163,184,0.25); }

  .fab-dock {
    position: absolute; right: 16px; bottom: 24px;
    display: flex; flex-direction: column; gap: 10px; z-index: 12;
  }
  .fab-btn {
    width: 48px; height: 48px; border-radius: 14px;
    background: var(--surface); border: 1px solid var(--border);
    color: var(--text); font-size: 20px;
    display: flex; align-items: center; justify-content: center;
    box-shadow: 0 4px 14px rgba(0,0,0,0.2);
  }
  .fab-btn:active { transform: scale(0.93); }

  .toast {
    position: fixed; bottom: 80px; left: 50%;
    transform: translateX(-50%) translateY(100px);
    background: var(--panel); color: var(--text);
    padding: 12px 20px; border-radius: 12px; font-size: 14px;
    box-shadow: 0 4px 20px rgba(0,0,0,0.4); z-index: 100;
    opacity: 0; transition: all 0.3s; pointer-events: none; max-width: 90vw;
  }
  .toast.show { opacity: 1; transform: translateX(-50%) translateY(0); }
</style>
</head>
<body>
<div id="network"></div>

<div id="spinner">
  <div class="ring"></div>
  <div class="text">Загрузка дерева...</div>
</div>

<div class="header" id="header" style="display:none">
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

<div class="fab-dock" id="fab-dock" style="display:none">
  <button class="fab-btn" title="К Венере" onclick="focusPerson('venera')">★</button>
  <button class="fab-btn" title="Показать всё" onclick="network.fit({animation:true})">⛶</button>
  <button class="fab-btn" id="theme-btn" title="Тема" onclick="toggleTheme()">☀️</button>
  <button class="fab-btn" title="PNG" onclick="exportPNG()">📷</button>
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
let nodesDS = null;
let edgesDS = null;
let currentBranch = 'all';
let selectedId = null;
let isLightTheme = false;
let pathNodes = new Set();
let pathEdges = new Set();
let pathPickSourceId = null;
let adjacencyCache = null;
let themeLock = false;

// 🔥 Палитры с учётом цветов рёбер
const palettes = {
  dark: {
    bg: '#0D1420', panel: '#172233', surface: 'rgba(23,34,51,0.95)',
    text: '#EAF0F8', muted: '#A3B4CC',
    male: '#6FB1F2', female: '#ED9FC1', gold: '#E9BD5A',
    path: '#34D399',
    edgeParent: '#677C99', edgeMarriage: '#ED9FC1', edgeSibling: '#859AB7'
  },
  light: {
    bg: '#F5F2EB', panel: '#EAE4D7', surface: 'rgba(234,228,215,0.95)',
    text: '#1E293B', muted: '#64748B',
    male: '#8FB3D9', female: '#EBA7BE', gold: '#D4A359',
    path: '#059669',
    edgeParent: '#8497B2', edgeMarriage: '#EBA7BE', edgeSibling: '#8FA5C0'
  }
};
let currentPalette = palettes.dark;

const container = document.getElementById('network');

const options = {
  layout: {
    improvedLayout: false,
    hierarchical: {
      direction: 'UD', sortMethod: 'directed',
      levelSeparation: 150, nodeSpacing: 160, treeSpacing: 200,
      blockShifting: false, edgeMinimization: false
    }
  },
  physics: false,
  interaction: {
    dragNodes: false, dragView: true, zoomView: true, hover: false,
    hideEdgesOnDrag: true, hideEdgesOnZoom: true
  },
  nodes: {
    shape: 'box',
    borderWidth: 1.5,
    borderWidthSelected: 3,
    margin: { top: 8, bottom: 8, left: 12, right: 12 },
    font: { color: '#1B283B', bold: true, size: 12, face: 'Segoe UI' }
  },
  edges: {
    smooth: { type: 'cubicBezier', forceDirection: 'vertical', roundness: 0.35 },
    width: 1.5
  },
  groups: {
    male:   { color: { background: '#6FB1F2', border: '#6FB1F2' }, font: { color: '#1B283B' } },
    female: { color: { background: '#ED9FC1', border: '#ED9FC1' }, font: { color: '#1B283B' } },
    center: { color: { background: '#E9BD5A', border: '#E9BD5A' }, font: { color: '#1B283B' } }
  }
};

function toast(msg, duration) {
  const el = document.getElementById('toast');
  el.textContent = msg;
  el.classList.add('show');
  setTimeout(() => el.classList.remove('show'), duration || 2500);
}

// ============================================================
// КЭШ УЗЛОВ (без color — цвет идёт от группы)
// ============================================================
function buildNodeData(p) {
  let label = p.name;
  if (p.id === 'venera') label += ' ★';
  let sub = [];
  if (p.maiden) sub.push('ур. ' + p.maiden);
  if (p.dates) sub.push(p.dates);
  if (sub.length > 0) label += '\n' + sub.join(' · ');

  return {
    id: p.id,
    label: label,
    level: p.generation,
    group: p.id === 'venera' ? 'center' : (p.sex === 'F' ? 'female' : 'male')
  };
}

function buildEdgeData(r, index) {
  const isMarriage = r.type === 'marriage';
  const color = r.type === 'parent' ? '#677C99'
              : (isMarriage ? '#ED9FC1' : '#859AB7');
  return {
    id: 'e_' + index,
    from: r.source,
    to: r.target,
    arrows: r.type === 'parent' ? { to: { enabled: true, scaleFactor: 0.5 } } : '',
    dashes: isMarriage,
    color: color,
    width: isMarriage ? 2 : 1.5
  };
}

function buildAdjacency() {
  adjacencyCache = {};
  familyData.relations.forEach((r, i) => {
    if (!adjacencyCache[r.source]) adjacencyCache[r.source] = [];
    if (!adjacencyCache[r.target]) adjacencyCache[r.target] = [];
    const id = 'e_' + i;
    adjacencyCache[r.source].push({ to: r.target, id });
    adjacencyCache[r.target].push({ to: r.source, id });
  });
}

// ============================================================
// ФИЛЬТР ПОИСКА + ВЕТВИ (обновляет свойства узлов)
// ============================================================
function renderGraph() {
  if (!nodesDS) return;
  const query = document.getElementById('search').value.toLowerCase().trim();
  const visible = {};
  let count = 0;

  // Видимость
  familyData.people.forEach(p => {
    const show = (currentBranch === 'all' || p.branch === currentBranch) &&
                 (!query || p.name.toLowerCase().includes(query));
    visible[p.id] = show;
    if (show) count++;
  });

  // Узлы
  const nodeUpdates = [];
  familyData.people.forEach(p => {
    const inPath = pathNodes.has(p.id);
    const isSearchMatch = query && p.name.toLowerCase().includes(query);
    const isDimmed = (query && !isSearchMatch) || (pathNodes.size > 0 && !inPath);
    let borderWidth = 1.5;
    if (inPath || isSearchMatch || p.id === 'venera') borderWidth = 3;
    nodeUpdates.push({
      id: p.id,
      hidden: !visible[p.id],
      opacity: isDimmed ? 0.2 : 1.0,
      borderWidth: borderWidth
    });
  });
  nodesDS.update(nodeUpdates);

  // Рёбра
  const edgeUpdates = [];
  familyData.relations.forEach((r, i) => {
    const show = visible[r.source] && visible[r.target];
    const inPath = pathEdges.has('e_' + i);
    const isMarriage = r.type === 'marriage';
    let color = r.type === 'parent' ? currentPalette.edgeParent
              : (isMarriage ? currentPalette.edgeMarriage : currentPalette.edgeSibling);
    if (inPath) color = currentPalette.path;
    else if (pathNodes.size > 0) color = 'rgba(148,163,184,0.15)';
    edgeUpdates.push({
      id: 'e_' + i,
      hidden: !show,
      color: color,
      width: inPath ? 4 : (isMarriage ? 2.2 : 1.5)
    });
  });
  edgesDS.update(edgeUpdates);

  document.getElementById('stat').textContent = count + ' персон';
}

// ============================================================
// БЫСТРОЕ ПЕРЕКЛЮЧЕНИЕ ТЕМЫ (без перебора всех 150 узлов)
// ============================================================
function toggleTheme() {
  if (themeLock) return;
  themeLock = true;

  isLightTheme = !isLightTheme;
  document.body.classList.toggle('light-theme', isLightTheme);
  currentPalette = isLightTheme ? palettes.light : palettes.dark;
  document.getElementById('theme-btn').textContent = isLightTheme ? '🌙' : '☀️';
  document.getElementById('meta-theme-color').content = currentPalette.bg;
  try { localStorage.setItem('ftree-theme', isLightTheme ? 'light' : 'dark'); } catch(e){}

  // 🔥 Мгновенно: меняем цвета групп (все узлы перекрасятся автоматически)
  network.setOptions({
    groups: {
      male:   { color: { background: currentPalette.male,   border: currentPalette.male   }, font: { color: '#1B283B' } },
      female: { color: { background: currentPalette.female, border: currentPalette.female }, font: { color: '#1B283B' } },
      center: { color: { background: currentPalette.gold,   border: currentPalette.gold   }, font: { color: '#1B283B' } }
    }
  });

  // 🔥 Перекрашиваем рёбра одним батчем
  const edgeUpdates = [];
  familyData.relations.forEach((r, i) => {
    const inPath = pathEdges.has('e_' + i);
    const isMarriage = r.type === 'marriage';
    let color = r.type === 'parent' ? currentPalette.edgeParent
              : (isMarriage ? currentPalette.edgeMarriage : currentPalette.edgeSibling);
    if (inPath) color = currentPalette.path;
    else if (pathNodes.size > 0) color = 'rgba(148,163,184,0.15)';
    edgeUpdates.push({ id: 'e_' + i, color: color });
  });
  edgesDS.update(edgeUpdates);

  // 🔥 Обновляем только выбранный узел (у него особый цвет рамки)
  if (selectedId) {
    nodesDS.update({
      id: selectedId,
      color: { border: isLightTheme ? '#334155' : '#FFFFFF' }
    });
  }

  // 🔥 Если открыта панель — обновляем её цвет
  if (selectedId) showPanel(selectedId);

  themeLock = false;
}

// ============================================================
// ПУТЬ РОДСТВА
// ============================================================
function findShortestPath(startId, targetId) {
  if (startId === targetId) return null;
  const prev = { [startId]: null };
  const queue = [startId];
  let head = 0;
  while (head < queue.length) {
    const curr = queue[head++];
    if (curr === targetId) break;
    for (const next of (adjacencyCache[curr] || [])) {
      if (!(next.to in prev)) {
        prev[next.to] = { from: curr, edge: next.id };
        queue.push(next.to);
      }
    }
  }
  if (!(targetId in prev)) return null;
  const nodes = [targetId], edges = [];
  let cur = targetId;
  while (cur !== startId) {
    const step = prev[cur]; edges.push(step.edge); cur = step.from; nodes.push(cur);
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
  setTimeout(() => network.fit({ nodes: res.nodes, animation: { duration: 400 } }), 50);
}

function startLink(id) {
  pathPickSourceId = id; closePanel();
  const p = familyData.people.find(x => x.id === id);
  document.getElementById('path-text').textContent = `👆 Нажмите на второго человека (от: ${p.name.split(' ')[0]})`;
  document.getElementById('path-banner').classList.add('active');
}

function clearPath() {
  pathNodes.clear(); pathEdges.clear(); pathPickSourceId = null;
  document.getElementById('path-banner').classList.remove('active');
  renderGraph();
}

// ============================================================
// ПАНЕЛЬ
// ============================================================
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

  const parents = [], spouses = [], children = [], siblings = [];
  for (const r of familyData.relations) {
    if (r.type === 'parent') {
      if (r.target === id) parents.push(r.source);
      if (r.source === id) children.push(r.target);
    } else if (r.source === id || r.target === id) {
      const other = r.source === id ? r.target : r.source;
      if (r.type === 'marriage') spouses.push(other);
      else if (r.type === 'sibling') siblings.push(other);
    }
  }

  let html = '';
  function addBlock(title, arr) {
    if (arr.length === 0) return;
    html += '<div class="rel-section"><div class="rel-title">' + title + ' · ' + arr.length + '</div><div class="rel-list">';
    arr.forEach(rid => {
      const rp = familyData.people.find(x => x.id === rid);
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
  network.focus(id, { scale: 1.0, animation: { duration: 400 } });
  network.selectNodes([id]);
  nodesDS.update({
    id: id,
    color: { border: isLightTheme ? '#334155' : '#FFFFFF' },
    borderWidth: 3
  });
  if (selectedId && selectedId !== id) {
    const old = familyData.people.find(x => x.id === selectedId);
    if (old) {
      const accent = old.id === 'venera' ? currentPalette.gold
                   : (old.sex === 'F' ? currentPalette.female : currentPalette.male);
      nodesDS.update({ id: old.id, color: { border: accent }, borderWidth: 1.5 });
    }
  }
  showPanel(id);
}

// ============================================================
// ЭКСПОРТ PNG
// ============================================================
function nextDraw(net) {
  return new Promise((resolve, reject) => {
    let to;
    net.once('afterDrawing', () => { clearTimeout(to); resolve(); });
    to = setTimeout(() => reject(new Error('Таймаут')), 8000);
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
    Math.max(1, Math.floor(2400 / dpr)) + 'px;';
  document.body.append(host);

  let temp = null;
  try {
    const exportNodes = familyData.people.map(buildNodeData);
    const exportEdges = familyData.relations.map(buildEdgeData);

    temp = new vis.Network(host, {
      nodes: new vis.DataSet(exportNodes),
      edges: new vis.DataSet(exportEdges)
    }, {
      layout: options.layout,
      physics: false,
      nodes: options.nodes,
      edges: options.edges,
      groups: {
        male:   { color: { background: currentPalette.male,   border: currentPalette.male   } },
        female: { color: { background: currentPalette.female, border: currentPalette.female } },
        center: { color: { background: currentPalette.gold,   border: currentPalette.gold   } }
      }
    });

    await nextDraw(temp);
    temp.fit({ animation: false });
    temp.moveTo({ scale: temp.getScale() * 0.85 });
    await nextDraw(temp);

    const srcCanvas = host.querySelector('canvas');
    const outCanvas = document.createElement('canvas');
    outCanvas.width = srcCanvas.width;
    outCanvas.height = srcCanvas.height;
    const ctx = outCanvas.getContext('2d');
    ctx.fillStyle = currentPalette.bg;
    ctx.fillRect(0, 0, outCanvas.width, outCanvas.height);
    ctx.drawImage(srcCanvas, 0, 0);
    ctx.fillStyle = currentPalette.muted;
    ctx.font = 'bold 22px "Segoe UI"';
    ctx.fillText('Семейное дерево · ' + familyData.people.length + ' персон', 28, outCanvas.height - 28);

    outCanvas.toBlob(async (blob) => {
      if (!blob) { toast('Не удалось создать PNG'); return; }
      const file = new File([blob], 'tree.png', { type: 'image/png' });
      if (navigator.canShare && navigator.canShare({ files: [file] })) {
        try { await navigator.share({ files: [file] }); return; } catch(e){}
      }
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url; a.download = 'tree.png'; a.click(); URL.revokeObjectURL(url);
    }, 'image/png');
  } catch(e) { toast('Ошибка: ' + e.message, 4000); }
  finally { if (temp) temp.destroy(); host.remove(); }
}

// ============================================================
// ИНИЦИАЛИЗАЦИЯ
// ============================================================
function init() {
  buildAdjacency();

  const initialNodes = familyData.people.map(buildNodeData);
  const initialEdges = familyData.relations.map(buildEdgeData);

  nodesDS = new vis.DataSet(initialNodes);
  edgesDS = new vis.DataSet(initialEdges);

  network = new vis.Network(container, { nodes: nodesDS, edges: edgesDS }, options);

  document.getElementById('spinner').style.display = 'none';
  document.getElementById('header').style.display = 'flex';
  document.getElementById('fab-dock').style.display = 'flex';

  network.fit({ animation: false });

  // Тема
  if (localStorage.getItem('ftree-theme') === 'light') {
    isLightTheme = true;
    document.body.classList.add('light-theme');
    currentPalette = palettes.light;
    document.getElementById('theme-btn').textContent = '🌙';
    document.getElementById('meta-theme-color').content = currentPalette.bg;
    network.setOptions({
      groups: {
        male:   { color: { background: currentPalette.male,   border: currentPalette.male   } },
        female: { color: { background: currentPalette.female, border: currentPalette.female } },
        center: { color: { background: currentPalette.gold,   border: currentPalette.gold   } }
      }
    });
    edgesDS.update(familyData.relations.map((r, i) => {
      const isMarriage = r.type === 'marriage';
      const color = r.type === 'parent' ? currentPalette.edgeParent
                  : (isMarriage ? currentPalette.edgeMarriage : currentPalette.edgeSibling);
      return { id: 'e_' + i, color: color };
    }));
  }

  document.getElementById('stat').textContent = familyData.people.length + ' персон';

  // Клик
  network.on('click', params => {
    if (params.nodes.length > 0) {
      const clicked = params.nodes[0];
      if (pathPickSourceId) {
        const src = pathPickSourceId;
        pathPickSourceId = null;
        applyPath(src, clicked);
        return;
      }
      focusPerson(clicked);
    } else { closePanel(); }
  });

  // Поиск (дебаунс)
  let st = null;
  document.getElementById('search').addEventListener('input', () => {
    clearTimeout(st);
    st = setTimeout(renderGraph, 200);
  });

  // Фильтры
  document.querySelectorAll('.filter-btn').forEach(btn => {
    btn.addEventListener('click', e => {
      document.querySelectorAll('.filter-btn').forEach(b => b.classList.remove('active'));
      e.target.classList.add('active');
      currentBranch = e.target.getAttribute('data-branch');
      renderGraph();
      setTimeout(() => network.fit({ animation: true }), 50);
    });
  });

  // Service Worker
  if ('serviceWorker' in navigator && location.protocol === 'https:') {
    navigator.serviceWorker.register('./sw.js', { scope: './' }).catch(err => console.warn(err));
  }
}

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', () => requestAnimationFrame(init));
} else {
  requestAnimationFrame(init);
}
</script>
</body>
</html>
"""

html = html.replace("__JSON_DATA__", json_str)
os.makedirs(MOBILE_DIR, exist_ok=True)
with open(os.path.join(MOBILE_DIR, "family_tree.html"), 'w', encoding='utf-8') as f:
    f.write(html)
print("✅ family_tree.html создан (быстрое переключение темы)")