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
<meta name="theme-color" content="#0D1420">
<link rel="manifest" href="manifest.json">
<title>Семейное дерево</title>
<script src="./vendor/vis-network.min.js"></script>
<style>
  :root {
    --bg: #0D1420;
    --panel: #172233;
    --text: #EAF0F8;
    --muted: #A3B4CC;
    --male: #6FB1F2;
    --female: #ED9FC1;
    --gold: #E9BD5A;
    --path: #34D399;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  html, body { height: 100%; background: var(--bg); color: var(--text);
    font-family: 'Segoe UI', Arial, sans-serif; overflow: hidden; }
  #network { width: 100%; height: 100%; }

  .header {
    position: absolute; top: 0; left: 0; right: 0;
    background: rgba(23, 34, 51, 0.95); padding: 12px 16px;
    display: flex; flex-direction: column; gap: 10px; z-index: 10;
    box-shadow: 0 4px 15px rgba(0,0,0,0.5);
    border-bottom-left-radius: 16px; border-bottom-right-radius: 16px;
  }
  .header h1 { font-size: 18px; font-weight: 600;
    display: flex; justify-content: space-between; align-items: center; }
  .search-box {
    width: 100%; padding: 10px 14px; border-radius: 10px; border: none;
    background: rgba(255,255,255,0.1); color: white; font-size: 15px;
    outline: none; min-height: 44px;
  }
  .search-box::placeholder { color: #8892a0; }
  .filters { display: flex; gap: 8px; overflow-x: auto; padding-bottom: 4px;
    scrollbar-width: none; }
  .filters::-webkit-scrollbar { display: none; }
  .filter-btn {
    background: rgba(255,255,255,0.1); border: none; color: var(--muted);
    padding: 8px 14px; border-radius: 20px; font-size: 13px;
    white-space: nowrap; cursor: pointer; min-height: 40px;
  }
  .filter-btn.active { background: var(--male); color: #0D1420; font-weight: 600; }

  .bottom-panel {
    position: absolute; bottom: 0; left: 0; right: 0;
    background: rgba(23, 34, 51, 0.98);
    border-top-left-radius: 24px; border-top-right-radius: 24px;
    padding: 20px; padding-bottom: 40px;
    transform: translateY(100%);
    transition: transform 0.3s cubic-bezier(0.175, 0.885, 0.32, 1.275);
    z-index: 20; max-height: 75vh; display: flex; flex-direction: column;
    box-shadow: 0 -4px 20px rgba(0,0,0,0.5);
  }
  .bottom-panel.open { transform: translateY(0); }
  .panel-handle { width: 40px; height: 5px; background: #4a5b75;
    border-radius: 3px; margin: 0 auto 15px auto; }
  .close-panel { position: absolute; top: 15px; right: 20px;
    background: none; border: none; color: var(--muted); font-size: 24px;
    cursor: pointer; }
  .person-header { border-bottom: 1px solid rgba(255,255,255,0.1);
    padding-bottom: 12px; margin-bottom: 12px; }
  .person-name { font-size: 22px; font-weight: bold; margin-bottom: 4px; }
  .person-meta { font-size: 13px; color: var(--muted); }
  .scroll-content { overflow-y: auto; flex: 1; padding-right: 5px; }
  .rel-section { margin-top: 12px; }
  .rel-title { font-size: 11px; text-transform: uppercase;
    color: var(--gold); margin-bottom: 6px; font-weight: bold;
    letter-spacing: 0.5px; }
  .rel-list { display: flex; flex-direction: column; gap: 6px; }
  .rel-item { background: rgba(255,255,255,0.05); padding: 12px;
    border-radius: 8px; font-size: 14px; cursor: pointer;
    display: flex; justify-content: space-between; min-height: 44px;
    align-items: center; }
  .rel-item:active { background: rgba(255,255,255,0.15); }

  .fab-dock {
    position: absolute; right: 16px; bottom: 24px;
    display: flex; flex-direction: column; gap: 10px; z-index: 12;
  }
  .fab-btn {
    width: 48px; height: 48px; border-radius: 14px;
    background: rgba(23, 34, 51, 0.9);
    border: 1px solid rgba(148, 163, 184, 0.25);
    color: var(--text); font-size: 20px; cursor: pointer;
    display: flex; align-items: center; justify-content: center;
    box-shadow: 0 4px 14px rgba(0,0,0,0.3);
    backdrop-filter: blur(10px);
    -webkit-backdrop-filter: blur(10px);
  }
  .fab-btn:active { transform: scale(0.93); }

  /* Toast для уведомлений */
  .toast {
    position: fixed; bottom: 80px; left: 50%;
    transform: translateX(-50%) translateY(100px);
    background: rgba(23, 34, 51, 0.98); color: var(--text);
    padding: 12px 20px; border-radius: 12px; font-size: 14px;
    box-shadow: 0 4px 20px rgba(0,0,0,0.4); z-index: 100;
    opacity: 0; transition: all 0.3s; pointer-events: none;
    max-width: 90vw;
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

<div class="fab-dock">
  <button class="fab-btn" title="К Венере" onclick="focusPerson('venera')">★</button>
  <button class="fab-btn" title="Показать всё" onclick="network.fit({animation:true})">⛶</button>
  <button class="fab-btn" title="Экспорт PNG" onclick="exportPNG()">📷</button>
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

<div class="toast" id="toast"></div>

<script>
const familyData = __JSON_DATA__;

let network = null;
let nodesDS = new vis.DataSet([]);
let edgesDS = new vis.DataSet([]);
let currentBranch = 'all';
let selectedId = null;

const container = document.getElementById('network');
const data = { nodes: nodesDS, edges: edgesDS };
const options = {
  layout: {
    hierarchical: {
      direction: 'UD',
      sortMethod: 'directed',
      levelSeparation: 150,
      nodeSpacing: 170,
      treeSpacing: 200,
      blockShifting: true,
      edgeMinimization: true
    }
  },
  physics: false,
  interaction: {
    dragNodes: false,
    dragView: true,
    zoomView: true,
    hover: false
  },
  nodes: {
    shape: 'box',
    margin: 12,
    font: { color: '#1B283B', bold: true, size: 14 },
    borderWidth: 0,
    shadow: { enabled: true, color: 'rgba(0,0,0,0.5)', size: 5, x: 0, y: 2 }
  },
  edges: {
    smooth: { type: 'cubicBezier', forceDirection: 'vertical', roundness: 0.4 },
    width: 1.5
  }
};

network = new vis.Network(container, data, options);

function toast(msg, duration) {
  const el = document.getElementById('toast');
  el.textContent = msg;
  el.classList.add('show');
  setTimeout(() => el.classList.remove('show'), duration || 2500);
}

function makeNode(p, exporting) {
  let color = p.id === 'venera' ? '#E9BD5A' : (p.sex === 'F' ? '#ED9FC1' : '#6FB1F2');
  let label = p.name;
  if (p.id === 'venera') label += '\n★';
  return {
    id: p.id,
    label: label,
    level: p.generation,
    color: { background: color, border: color },
    font: { color: '#1B283B', bold: true, size: 13 },
    widthConstraint: { maximum: 160 }
  };
}

function makeEdge(r, index) {
  const isMarriage = r.type === 'marriage';
  return {
    id: 'e_' + index,
    from: r.source,
    to: r.target,
    arrows: r.type === 'parent' ? 'to' : '',
    dashes: isMarriage,
    color: r.type === 'parent' ? '#677C99' : (isMarriage ? '#ED9FC1' : '#859AB7'),
    width: isMarriage ? 2 : 1.5
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
      nodesDS.add(makeNode(p, false));
    }
  });

  familyData.relations.forEach((r, i) => {
    if (visibleNodes.has(r.source) && visibleNodes.has(r.target)) {
      edgesDS.add(makeEdge(r, i));
    }
  });

  document.getElementById('stat').textContent = nodesDS.length + ' персон';
}

network.on('click', params => {
  if (params.nodes.length > 0) {
    showPanel(params.nodes[0]);
  } else {
    closePanel();
  }
});

function showPanel(id) {
  selectedId = id;
  const p = familyData.people.find(x => x.id === id);
  if (!p) return;

  document.getElementById('p-name').textContent = p.name;
  document.getElementById('p-name').style.color =
    p.id === 'venera' ? '#E9BD5A' : (p.sex === 'F' ? '#ED9FC1' : '#6FB1F2');

  let meta = [];
  if (p.dates) meta.push(p.dates);
  if (p.branch) meta.push(p.branch === 'paternal' ? 'Отцовская'
    : p.branch === 'maternal' ? 'Материнская' : 'Центр');
  document.getElementById('p-meta').textContent = meta.join(' · ');

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
// ЭКСПОРТ PNG (всё дерево, а не экран)
// ============================================================
function nextDraw(net) {
  return new Promise((resolve, reject) => {
    let timeout;
    const done = () => { clearTimeout(timeout); resolve(); };
    net.once('afterDrawing', done);
    timeout = setTimeout(() => {
      net.off('afterDrawing', done);
      reject(new Error('Таймаут отрисовки'));
    }, 6000);
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
    const allNodes = familyData.people.map(p => makeNode(p, true));
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

    if (srcCanvas.width * srcCanvas.height > 12000000) {
      throw new Error('Слишком большой canvas для этого телефона');
    }

    const outCanvas = document.createElement('canvas');
    outCanvas.width = srcCanvas.width;
    outCanvas.height = srcCanvas.height;
    const ctx = outCanvas.getContext('2d');
    if (!ctx) throw new Error('Нет памяти');

    ctx.fillStyle = '#0D1420';
    ctx.fillRect(0, 0, outCanvas.width, outCanvas.height);
    ctx.drawImage(srcCanvas, 0, 0);

    ctx.fillStyle = '#A3B4CC';
    ctx.font = 'bold 22px "Segoe UI", sans-serif';
    ctx.fillText('Семейное дерево · ' + familyData.people.length + ' персон',
                 28, outCanvas.height - 28);

    outCanvas.toBlob(async (blob) => {
      if (!blob) { toast('Не удалось создать PNG'); return; }
      const file = new File([blob], 'family_tree.png', { type: 'image/png' });

      if (navigator.canShare && navigator.canShare({ files: [file] })) {
        try {
          await navigator.share({ files: [file], title: 'Семейное дерево' });
          toast('Отправлено!');
          return;
        } catch (e) {
          if (e.name === 'AbortError') return;
        }
      }

      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = 'family_tree.png';
      document.body.appendChild(a);
      a.click();
      a.remove();
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

// Service Worker для офлайна
if ('serviceWorker' in navigator && location.protocol === 'https:') {
  window.addEventListener('load', () => {
    navigator.serviceWorker.register('./sw.js', { scope: './' })
      .catch(err => console.warn('SW не зарегистрирован:', err));
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

# --- Service Worker ---
sw = r"""const CACHE = 'family-tree-v1';
const ASSETS = ['./', './family_tree.html', './manifest.json',
  './vendor/vis-network.min.js'];

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

# --- Manifest ---
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

print("\n📦 Готово. Структура mobile/:")
for f in sorted(os.listdir(MOBILE_DIR)):
    full = os.path.join(MOBILE_DIR, f)
    if os.path.isfile(full):
        print(f"   {f} ({os.path.getsize(full) // 1024} KB)")