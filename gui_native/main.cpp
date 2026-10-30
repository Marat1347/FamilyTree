// FamilyTree GUI viewer.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define main ftree_console_entry_not_used
#include "../ftree.cpp"
#undef main
#include <windowsx.h>
#include <gdiplus.h>
#include <commdlg.h>
#include <memory>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <regex>
#include <algorithm>
#define IDI_APPICON 101

using namespace Gdiplus;

namespace Ui {
    inline Color background() { return Color(255, 13, 20, 32); }
    inline Color panel()      { return Color(255, 23, 34, 51); }
    inline Color text()       { return Color(255, 234, 240, 248); }
    inline Color muted()      { return Color(255, 163, 180, 204); }
    inline Color male()       { return Color(255, 111, 177, 242); }
    inline Color female()     { return Color(255, 237, 159, 193); }
    inline Color gold()       { return Color(255, 233, 189, 90); }
    inline Color green()      { return Color(255, 120, 200, 150); }
    inline Color emerald()    { return Color(255, 52, 211, 153); }

    inline Color accent(const Person& p) {
        if (p.id == "venera") return gold();
        if (p.sex == "F") return female();
        return male();
    }

    inline RectF expanded(RectF r, REAL amount) {
        r.Inflate(amount, amount);
        return r;
    }

    inline void rounded(GraphicsPath& path, const RectF& r, REAL radius = 14) {
        const REAL d = radius * 2;
        path.AddArc(r.X, r.Y, d, d, 180, 90);
        path.AddArc(r.GetRight() - d, r.Y, d, d, 270, 90);
        path.AddArc(r.GetRight() - d, r.GetBottom() - d, d, d, 0, 90);
        path.AddArc(r.X, r.GetBottom() - d, d, d, 90, 90);
        path.CloseFigure();
    }

    inline REAL textHeight(Graphics& g, const std::wstring& value,
                           REAL width, REAL size, bool bold = false) {
        if (value.empty()) return 0;
        Gdiplus::Font font(L"Segoe UI", size,
                           bold ? FontStyleBold : FontStyleRegular, UnitPixel);
        StringFormat format;
        RectF bounds;
        g.MeasureString(value.c_str(), (INT)value.size(), &font,
                        RectF(0, 0, width, 10000), &format, &bounds);
        return std::ceil(bounds.Height);
    }

    inline void label(Graphics& g, const std::wstring& value, const RectF& rect,
                      REAL size, Color color, bool bold = false, bool wrap = false) {
        if (value.empty()) return;
        Gdiplus::Font font(L"Segoe UI", size,
                           bold ? FontStyleBold : FontStyleRegular, UnitPixel);
        SolidBrush brush(color);
        StringFormat format;
        if (!wrap) format.SetFormatFlags(StringFormatFlagsNoWrap);
        format.SetTrimming(StringTrimmingNone);
        g.DrawString(value.c_str(), (INT)value.size(), &font, rect, &format, &brush);
    }

    inline std::wstring branchName(const std::string& branch) {
        if (branch == "paternal") return L"Отцовская ветвь";
        if (branch == "maternal") return L"Материнская ветвь";
        if (branch == "center")   return L"Центральная ветвь";
        return L"Ветвь не указана";
    }
}

static std::unique_ptr<FamilyTree> model;
static HWND mainWindow, canvas, searchBox, peopleList, detailBox;
static HFONT uiFont;
static std::vector<std::string> listIds;
static std::string selected;
static float zoom = .65f, panX = 30, panY = 70;
static bool dragging = false, moved = false;
static POINT mouseStart, mouseLast;
static HBRUSH panelBrush;
struct Card { std::string id; RectF rect; };
static std::vector<Card> cards;
static float treeWidth = 1, treeHeight = 1;

static int detailsScroll = 0;
static int detailsContentHeight = 0;
struct DetailHit { std::string id; RectF rect; };
static std::vector<DetailHit> detailHits;

static std::string hoveredCard;
static std::string hoveredDetail;
static POINT mouseCanvasPt = { 0, 0 };
static std::wstring currentSearchQuery;
static std::string currentBranchFilter;
static std::vector<std::string> highlightedPath;

static std::wstring wide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

static std::wstring text(HWND h) {
    int n = GetWindowTextLengthW(h);
    std::wstring s(n + 1, L'\0');
    GetWindowTextW(h, s.data(), n + 1);
    s.resize(n);
    return s;
}

static UINT windowDpi(HWND h) {
    HDC dc = GetDC(h);
    UINT dpi = (UINT)GetDeviceCaps(dc, LOGPIXELSX);
    ReleaseDC(h, dc);
    return dpi ? dpi : 96;
}

static void collectRelatives(const std::string& id,
                              std::vector<std::string>& parents,
                              std::vector<std::string>& spouses,
                              std::vector<std::string>& children,
                              std::vector<std::string>& siblings) {
    parents.clear(); spouses.clear(); children.clear(); siblings.clear();
    for (const auto& rel : model->relations) {
        if (rel.type == "parent") {
            if (rel.tgt == id) parents.push_back(rel.src);
            if (rel.src == id) children.push_back(rel.tgt);
        } else if (rel.src == id || rel.tgt == id) {
            const std::string other = (rel.src == id) ? rel.tgt : rel.src;
            if (rel.type == "marriage") spouses.push_back(other);
            else if (rel.type == "sibling") siblings.push_back(other);
        }
    }
}

static bool matchesSearch(const Person& p) {
    if (currentSearchQuery.empty()) return false;
    auto name = wide(p.name);
    if (!name.empty()) CharLowerBuffW(name.data(), (DWORD)name.size());
    return name.find(currentSearchQuery) != std::wstring::npos;
}

static bool matchesBranchFilter(const Person& p) {
    if (currentBranchFilter.empty()) return true;
    return p.branch == currentBranchFilter;
}

static REAL cardHeight(Graphics& g, const Person& p, REAL width) {
    const REAL textWidth = width - 48;
    const REAL nameH = Ui::textHeight(g, wide(p.name), textWidth, 20, true);
    const REAL maidenH = p.maiden.empty() ? 0 :
        Ui::textHeight(g, L"урожд. " + wide(p.maiden), textWidth, 13) + 5;
    return std::max(164.f, 60.f + nameH + 10.f + maidenH + 22.f
                    + (p.dates.empty() ? 0.f : 22.f) + 18.f);
}

static void arrange() {
    cards.clear();
    std::map<int, std::vector<Person*>> rows;
    for (auto& p : model->people)
        if (matchesBranchFilter(p)) rows[p.gen].push_back(&p);

    if (rows.empty()) {
        treeWidth = 400;
        treeHeight = 200;
        return;
    }

    HDC dc = GetDC(canvas);
    Graphics measure(dc);
    const REAL cardWidth = 340;
    const REAL gapX = 40;
    const REAL gapY = 96;

    size_t maxCount = 1;
    for (const auto& row : rows) maxCount = std::max(maxCount, row.second.size());
    treeWidth = (REAL)maxCount * (cardWidth + gapX) + 64;

    REAL y = 110;
    for (auto& row : rows) {
        std::sort(row.second.begin(), row.second.end(), [](Person* a, Person* b) {
            return a->branch != b->branch ? a->branch < b->branch : a->name < b->name;
        });
        REAL rowHeight = 164;
        for (const auto* person : row.second)
            rowHeight = std::max(rowHeight, cardHeight(measure, *person, cardWidth));

        const REAL rowWidth = (REAL)row.second.size() * cardWidth +
            (REAL)(row.second.empty() ? 0 : row.second.size() - 1) * gapX;
        const REAL startX = (treeWidth - rowWidth) / 2;

        for (size_t i = 0; i < row.second.size(); ++i) {
            cards.push_back({ row.second[i]->id,
                RectF(startX + (REAL)i * (cardWidth + gapX), y, cardWidth, rowHeight) });
        }
        y += rowHeight + gapY;
    }
    treeHeight = y + 40;
    ReleaseDC(canvas, dc);
}

static void fit() {
    RECT r; GetClientRect(canvas, &r);
    zoom = std::clamp(std::min((r.right - 50.f) / treeWidth, (r.bottom - 50.f) / treeHeight), .025f, 1.2f);
    panX = (r.right - treeWidth * zoom) / 2;
    panY = (r.bottom - treeHeight * zoom) / 2;
    InvalidateRect(canvas, nullptr, FALSE);
}

static void selectPerson(const std::string& id, bool center) {
    selected = id;
    detailsScroll = 0;
    InvalidateRect(detailBox, nullptr, FALSE);
    if (center) {
        RECT r; GetClientRect(canvas, &r);
        for (auto& card : cards) if (card.id == id) {
            zoom = .9f;
            panX = r.right / 2.f - (card.rect.X + card.rect.Width / 2) * zoom;
            panY = r.bottom / 2.f - (card.rect.Y + card.rect.Height / 2) * zoom;
        }
    }
    InvalidateRect(canvas, nullptr, FALSE);
}

static void filterList() {
    auto query = text(searchBox);
    if (!query.empty()) CharLowerBuffW(query.data(), (DWORD)query.size());
    currentSearchQuery = query;
    InvalidateRect(canvas, nullptr, FALSE);

    std::vector<Person*> result;
    for (auto& p : model->people) {
        if (!matchesBranchFilter(p)) continue;
        auto name = wide(p.name);
        if (!name.empty()) CharLowerBuffW(name.data(), (DWORD)name.size());
        if (query.empty() || name.find(query) != std::wstring::npos) result.push_back(&p);
    }
    std::sort(result.begin(), result.end(), [](Person* a, Person* b) {
        if (a->gen != b->gen) return a->gen < b->gen;
        return a->name < b->name;
    });
    SendMessageW(peopleList, LB_RESETCONTENT, 0, 0);
    listIds.clear();
    int curGen = -1;
    for (auto* p : result) {
        if (p->gen != curGen) {
            curGen = p->gen;
            auto header = L"─── ПОКОЛЕНИЕ " + std::to_wstring(curGen) + L" ───";
            listIds.push_back("");
            SendMessageW(peopleList, LB_ADDSTRING, 0, (LPARAM)header.c_str());
        }
        auto w = wide(p->name);
        listIds.push_back(p->id);
        SendMessageW(peopleList, LB_ADDSTRING, 0, (LPARAM)w.c_str());
    }
}

static void applyBranchFilter(const std::string& branch) {
    currentBranchFilter = branch;
    HWND btn = GetDlgItem(mainWindow, 106);
    if (btn) {
        std::wstring label = L"Ветвь: ";
        if (branch.empty()) label += L"Все";
        else if (branch == "paternal") label += L"Отцовская";
        else if (branch == "maternal") label += L"Материнская";
        else if (branch == "center") label += L"Центр";
        label += L"  ▾";
        SetWindowTextW(btn, label.c_str());
        InvalidateRect(btn, nullptr, FALSE);
    }
    arrange();
    fit();
    filterList();
}

static std::vector<std::string> computePath(const std::string& from, const std::string& to) {
    std::vector<std::string> path;
    if (from.empty() || to.empty() || from == to) return path;
    std::map<std::string, std::vector<std::string>> adj;
    for (const auto& r : model->relations) {
        adj[r.src].push_back(r.tgt);
        adj[r.tgt].push_back(r.src);
    }
    std::map<std::string, std::string> prev;
    std::vector<std::string> q = { from };
    prev[from] = "";
    for (size_t i = 0; i < q.size(); ++i) {
        if (q[i] == to) break;
        for (const auto& nx : adj[q[i]]) {
            if (!prev.count(nx)) { prev[nx] = q[i]; q.push_back(nx); }
        }
    }
    if (!prev.count(to)) return path;
    std::string cur = to;
    while (!cur.empty()) { path.push_back(cur); cur = prev[cur]; }
    std::reverse(path.begin(), path.end());
    return path;
}

static bool edgeInPath(const std::string& a, const std::string& b) {
    for (size_t i = 0; i + 1 < highlightedPath.size(); ++i) {
        if ((highlightedPath[i] == a && highlightedPath[i+1] == b) ||
            (highlightedPath[i] == b && highlightedPath[i+1] == a)) return true;
    }
    return false;
}

static std::set<std::string> getTrunkNodes() {
    std::set<std::string> trunk;
    if (!model || !model->find("venera")) return trunk;
    trunk.insert("venera");
    std::vector<std::string> qUp = { "venera" };
    for (size_t i = 0; i < qUp.size(); ++i)
        for (const auto& rel : model->relations)
            if (rel.type == "parent" && rel.tgt == qUp[i] && trunk.insert(rel.src).second)
                qUp.push_back(rel.src);
    std::vector<std::string> qDown = { "venera" };
    for (size_t i = 0; i < qDown.size(); ++i)
        for (const auto& rel : model->relations)
            if (rel.type == "parent" && rel.src == qDown[i] && trunk.insert(rel.tgt).second)
                qDown.push_back(rel.tgt);
    return trunk;
}

static RectF getMinimapRect(REAL screenW, REAL screenH) {
    const REAL mapW = 190.f;
    const REAL mapH = std::clamp(mapW * (treeHeight / std::max(1.f, treeWidth)), 80.f, 130.f);
    return RectF(screenW - mapW - 16.f, screenH - mapH - 46.f, mapW, mapH);
}

static void drawMinimap(Graphics& g, REAL screenW, REAL screenH, const RECT& clientPx) {
    if (screenW < 600 || screenH < 450 || cards.empty()) return;
    RectF mm = getMinimapRect(screenW, screenH);
    GraphicsPath mmPath;
    Ui::rounded(mmPath, mm, 8);
    SolidBrush mmBg(Color(215, 16, 24, 38));
    Pen mmBorder(Color(90, 143, 166, 196), 1.f);
    g.FillPath(&mmBg, &mmPath);
    g.DrawPath(&mmBorder, &mmPath);

    const REAL pad = 8.f;
    const REAL scale = std::min((mm.Width - pad * 2) / std::max(1.f, treeWidth),
                                (mm.Height - pad * 2) / std::max(1.f, treeHeight));
    const REAL ox = mm.X + (mm.Width - treeWidth * scale) / 2.f;
    const REAL oy = mm.Y + (mm.Height - treeHeight * scale) / 2.f;

    for (const auto& c : cards) {
        auto* p = model->find(c.id);
        if (!p) continue;
        RectF cr(ox + c.rect.X * scale, oy + c.rect.Y * scale,
                 std::max(3.f, c.rect.Width * scale), std::max(2.f, c.rect.Height * scale));
        SolidBrush cb(c.id == selected ? Color(255, 255, 255, 255) : Ui::accent(*p));
        g.FillRectangle(&cb, cr);
    }

    REAL vx = (-panX / zoom) * scale + ox;
    REAL vy = (-panY / zoom) * scale + oy;
    REAL vw = ((REAL)clientPx.right / zoom) * scale;
    REAL vh = ((REAL)clientPx.bottom / zoom) * scale;
    RectF viewRect(vx, vy, vw, vh);
    viewRect.Intersect(mm);

    SolidBrush viewFill(Color(28, 255, 255, 255));
    Pen viewPen(Color(200, 233, 189, 90), 1.2f);
    g.FillRectangle(&viewFill, viewRect);
    g.DrawRectangle(&viewPen, viewRect);
}

static void drawPersonCard(Graphics& g, const Person& p, const RectF& r,
                           bool isSelected, bool detailed) {
    const Color accent = Ui::accent(p);
    const bool isHovered = (p.id == hoveredCard);
    const bool isSearchHit = matchesSearch(p);
    const bool isDimmed = (!currentSearchQuery.empty() && !isSearchHit && !isSelected);

    if (p.id == "venera") {
        RectF glowRect = Ui::expanded(r, 30.f);
        GraphicsPath glowPath;
        glowPath.AddEllipse(glowRect);
        PathGradientBrush pgb(&glowPath);
        pgb.SetCenterColor(Color(60, 233, 189, 90));
        Color surround[1] = { Color(0, 233, 189, 90) };
        INT count = 1;
        pgb.SetSurroundColors(surround, &count);
        g.FillPath(&pgb, &glowPath);
    }

    if (p.id == "venera") {
        GraphicsPath outer;
        Ui::rounded(outer, Ui::expanded(r, 4), 18);
        Pen halo(Color(70, 233, 189, 90), 2.f);
        g.DrawPath(&halo, &outer);
    }

    if (detailed || isHovered) {
        int layers = isHovered ? 6 : 4;
        BYTE shadowAlpha = isHovered ? 16 : 9;
        int yShift = isHovered ? 8 : 5;
        for (int i = layers; i >= 1; --i) {
            RectF shadowRect = Ui::expanded(r, (REAL)(i * 1.8f));
            shadowRect.Y += yShift;
            GraphicsPath shadow;
            Ui::rounded(shadow, shadowRect, 16);
            SolidBrush brush(Color(shadowAlpha, 0, 0, 0));
            g.FillPath(&brush, &shadow);
        }
    }

    GraphicsPath shape;
    Ui::rounded(shape, r, 14);
    LinearGradientBrush fill(PointF(r.X, r.Y), PointF(r.X, r.GetBottom()),
                             Color(255, 36, 51, 73), Color(255, 27, 40, 59));
    g.FillPath(&fill, &shape);

    Color borderColor = isSelected ? accent
                      : isSearchHit ? Color(255, 255, 215, 0)
                      : isHovered ? Color(255, 130, 165, 210)
                      : Color(255, 58, 76, 100);
    REAL borderWidth = (isSelected || isSearchHit) ? 2.6f : (isHovered ? 1.8f : 1.f);
    Pen border(borderColor, borderWidth);
    g.DrawPath(&border, &shape);

    Pen highlight(Color(35, 255, 255, 255), 1.f);
    g.DrawLine(&highlight, r.X + 18, r.Y + 1, r.GetRight() - 18, r.Y + 1);

    SolidBrush dot(accent);
    g.FillEllipse(&dot, r.X + 20, r.Y + 20, 7.f, 7.f);

    Ui::label(g, p.sex == "F" ? L"ЖЕНЩИНА" : p.sex == "M" ? L"МУЖЧИНА" : L"ПЕРСОНА",
              RectF(r.X + 34, r.Y + 14, 135, 18), 10, Ui::muted(), true);

    if (p.id == "venera") {
        const RectF badge(r.GetRight() - 94, r.Y + 11, 78, 24);
        GraphicsPath badgeShape;
        Ui::rounded(badgeShape, badge, 8);
        SolidBrush fillBadge(Color(35, 233, 189, 90));
        g.FillPath(&fillBadge, &badgeShape);
        Ui::label(g, L"ЦЕНТР",
                  RectF(badge.X + 15, badge.Y + 4, badge.Width - 20, 18),
                  10, Ui::gold(), true);
    }

    const REAL x = r.X + 24;
    const REAL textWidth = r.Width - 48;
    REAL y = r.Y + 42;

    const REAL nameH = Ui::textHeight(g, wide(p.name), textWidth, 20, true);
    Ui::label(g, wide(p.name), RectF(x, y, textWidth, nameH + 2), 20, Ui::text(), true, true);

    if (!detailed) {
        if (isDimmed) {
            SolidBrush dim(Color(165, 13, 20, 32));
            g.FillPath(&dim, &shape);
        }
        return;
    }

    y += nameH + 10;

    if (!p.maiden.empty()) {
        const std::wstring maiden = L"урожд. " + wide(p.maiden);
        const REAL h = Ui::textHeight(g, maiden, textWidth, 13);
        Ui::label(g, maiden, RectF(x, y, textWidth, h + 2), 13, Ui::muted(), false, true);
        y += h + 5;
    }

    Ui::label(g, Ui::branchName(p.branch), RectF(x, y, textWidth, 21), 12, Ui::muted());
    y += 22;

    if (!p.dates.empty())
        Ui::label(g, wide(p.dates), RectF(x, y, textWidth, 21), 13, Ui::text());

    if (isDimmed) {
        SolidBrush dim(Color(165, 13, 20, 32));
        g.FillPath(&dim, &shape);
    }
}

static void drawGenerationGuides(Graphics& g) {
    struct RowBounds { REAL left, right, top; };
    std::map<int, RowBounds> rows;
    for (const auto& card : cards) {
        auto* person = model->find(card.id);
        if (!person) continue;
        auto it = rows.find(person->gen);
        if (it == rows.end()) {
            rows.emplace(person->gen,
                RowBounds{ card.rect.X, card.rect.GetRight(), card.rect.Y });
        } else {
            it->second.left = std::min(it->second.left, card.rect.X);
            it->second.right = std::max(it->second.right, card.rect.GetRight());
            it->second.top = std::min(it->second.top, card.rect.Y);
        }
    }
    for (const auto& entry : rows) {
        const auto& row = entry.second;
        const REAL y = row.top - 34;
        Ui::label(g, L"ПОКОЛЕНИЕ " + std::to_wstring(entry.first),
                  RectF(row.left, y, 145, 20), 11, Ui::muted(), true);
        if (row.right > row.left + 160) {
            Pen line(Color(55, 163, 180, 204), 1.f);
            g.DrawLine(&line, row.left + 150, y + 9, row.right, y + 9);
        }
    }
}

static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    std::vector<BYTE> buf(size);
    auto* pImageCodecInfo = reinterpret_cast<ImageCodecInfo*>(buf.data());
    GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            return (int)j;
        }
    }
    return -1;
}

static void renderTreeContent(Graphics& g, float currentZoom) {
    std::map<std::string, RectF> boxes;
    for (const auto& c : cards) boxes[c.id] = c.rect;
    drawGenerationGuides(g);
    const bool hasSelection = !selected.empty();
    const auto trunkNodes = getTrunkNodes();

    for (const auto& rel : model->relations) {
        if (!boxes.count(rel.src) || !boxes.count(rel.tgt)) continue;
        const auto a = boxes.at(rel.src);
        const auto b = boxes.at(rel.tgt);
        const bool focused = hasSelection && (rel.src == selected || rel.tgt == selected);
        const bool isTrunkEdge = trunkNodes.count(rel.src) && trunkNodes.count(rel.tgt);
        const bool inPath = edgeInPath(rel.src, rel.tgt);
        if (rel.type == "sibling" && !focused && !inPath) continue;

        const BYTE alpha = !hasSelection ? 175 : focused ? 255 : 45;
        Color color(alpha, 143, 166, 196);
        REAL width = focused ? 2.8f : 1.6f;
        GraphicsPath path;

        if (rel.type == "parent") {
            const PointF start(a.X + a.Width / 2, a.GetBottom() + 3);
            const PointF end(b.X + b.Width / 2, b.Y - 7);
            const REAL bend = (end.Y - start.Y) * 0.48f;
            path.AddBezier(start, PointF(start.X, start.Y + bend),
                           PointF(end.X, end.Y - bend), end);

            if (inPath) {
                color = Ui::emerald();
                width = 4.0f;
            } else if (isTrunkEdge && (!hasSelection || focused)) {
                Pen trunkGlow(Color(50, 233, 189, 90), 8.f);
                trunkGlow.SetStartCap(LineCapRound);
                trunkGlow.SetEndCap(LineCapRound);
                g.DrawPath(&trunkGlow, &path);
                color = Color(235, 233, 189, 90);
                width = focused ? 3.4f : 2.6f;
            } else if (focused) {
                color = Color(255, 210, 225, 245);
            }
            Pen pen(color, width);
            g.DrawPath(&pen, &path);
        } else {
            const bool leftToRight = a.X + a.Width / 2 < b.X + b.Width / 2;
            const PointF start(leftToRight ? a.GetRight() + 4 : a.X - 4, a.Y + a.Height / 2);
            const PointF end(leftToRight ? b.X - 4 : b.GetRight() + 4, b.Y + b.Height / 2);
            const REAL middleX = (start.X + end.X) / 2;
            path.AddBezier(start, PointF(middleX, start.Y),
                           PointF(middleX, end.Y), end);

            if (inPath) {
                color = Ui::emerald();
                width = 3.5f;
            }
            Pen pen(rel.type == "marriage" ? Color(color.GetA(), 194, 137, 165)
                                           : Color(color.GetA(), 133, 154, 183),
                    focused ? 2.2f : 1.4f);
            if (inPath) pen.SetColor(color), pen.SetWidth(width), pen.SetDashStyle(DashStyleSolid);
            else pen.SetDashStyle(rel.type == "marriage" ? DashStyleDash : DashStyleDot);
            g.DrawPath(&pen, &path);
        }
    }

    for (const auto& card : cards) {
        auto* person = model->find(card.id);
        if (!person) continue;
        drawPersonCard(g, *person, card.rect, card.id == selected, currentZoom >= 0.55f);
    }
}

static void exportPng() {
    wchar_t fileName[MAX_PATH] = L"Семейное_дерево.png";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = mainWindow;
    ofn.lpstrFilter = L"Изображение PNG (*.png)\0*.png\0Все файлы (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"png";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&ofn)) return;

    const float margin = 48.f;
    float exportScale = 1.0f;
    int bmpW = (int)((treeWidth + margin * 2) * exportScale);
    int bmpH = (int)((treeHeight + margin * 2) * exportScale);
    if (bmpW > 8192) {
        exportScale = 8192.f / (treeWidth + margin * 2);
        bmpW = (int)((treeWidth + margin * 2) * exportScale);
        bmpH = (int)((treeHeight + margin * 2) * exportScale);
    }

    Bitmap bmp(bmpW, bmpH, PixelFormat32bppPARGB);
    if (bmp.GetLastStatus() != Ok) {
        MessageBoxW(mainWindow, L"Недостаточно памяти для PNG.", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    Graphics g(&bmp);
    g.Clear(Ui::background());
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    g.TranslateTransform(margin * exportScale, margin * exportScale);
    g.ScaleTransform(exportScale, exportScale);
    renderTreeContent(g, 1.0f);

    CLSID pngClsid;
    if (GetEncoderClsid(L"image/png", &pngClsid) >= 0
        && bmp.Save(fileName, &pngClsid, nullptr) == Ok) {
        MessageBoxW(mainWindow, L"Дерево сохранено в PNG!", L"Готово", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxW(mainWindow, L"Не удалось сохранить PNG.", L"Ошибка", MB_OK | MB_ICONERROR);
    }
}

// ============================================================
// GEDCOM 5.5.1 ЭКСПОРТ
// ============================================================
struct LifespanInfo {
    int birthYear = 0;
    int deathYear = 0;
    int age = -1;
    bool isAlive = false;
};

static LifespanInfo parseDates(const std::string& datesStr) {
    LifespanInfo info;
    if (datesStr.empty()) return info;
    std::vector<int> years;
    std::regex re("([12][089][0-9]{2})");
    auto begin = std::sregex_iterator(datesStr.begin(), datesStr.end(), re);
    for (auto it = begin; it != std::sregex_iterator(); ++it) {
        try { years.push_back(std::stoi(it->str())); } catch (...) {}
    }
    if (years.size() >= 2) {
        info.birthYear = years[0];
        info.deathYear = years[1];
        if (info.deathYear >= info.birthYear) info.age = info.deathYear - info.birthYear;
    } else if (years.size() == 1) {
        info.birthYear = years[0];
        info.isAlive = (datesStr.find("р.") != std::string::npos);
        if (info.isAlive && info.birthYear <= 2026) info.age = 2026 - info.birthYear;
    }
    return info;
}

static void exportGedcom() {
    wchar_t fileName[MAX_PATH] = L"Семейное_дерево.ged";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = mainWindow;
    ofn.lpstrFilter = L"Файл GEDCOM 5.5.1 (*.ged)\0*.ged\0Все файлы (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"ged";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&ofn)) return;

    std::ofstream f(fs::path(fileName), std::ios::binary | std::ios::trunc);
    if (!f) {
        MessageBoxW(mainWindow, L"Не удалось создать GEDCOM файл.", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    f << "0 HEAD\n1 CHAR UTF-8\n1 GEDC\n2 VERS 5.5.1\n2 FORM LINEAGE-LINKED\n1 SOUR FamilyTree_Win32\n";

    for (const auto& p : model->people) {
        f << "0 @I_" << p.id << "@ INDI\n";
        std::string given = p.name, surname = "";
        size_t sp = p.name.find(' ');
        if (sp != std::string::npos) {
            given = p.name.substr(0, sp);
            surname = p.name.substr(sp + 1);
        }
        if (!p.maiden.empty()) surname = p.maiden;
        f << "1 NAME " << given << " /" << surname << "/\n";
        if (!p.sex.empty()) f << "1 SEX " << p.sex << "\n";

        auto lf = parseDates(p.dates);
        if (lf.birthYear > 0) f << "1 BIRT\n2 DATE " << lf.birthYear << "\n";
        if (lf.deathYear > 0) f << "1 DEAT\n2 DATE " << lf.deathYear << "\n";
    }

    std::map<std::pair<std::string, std::string>, std::vector<std::string>> famKids;
    std::set<std::pair<std::string, std::string>> marriages;

    for (const auto& r : model->relations) {
        if (r.type == "marriage") {
            auto a = r.src, b = r.tgt;
            if (auto* pa = model->find(a); pa && pa->sex == "F") std::swap(a, b);
            marriages.insert({a, b});
        }
    }
    for (const auto& p : model->people) {
        std::vector<std::string> par, spo, chi, sib;
        collectRelatives(p.id, par, spo, chi, sib);
        if (par.size() >= 2) {
            std::string h = par[0], w = par[1];
            if (auto* ph = model->find(h); ph && ph->sex == "F") std::swap(h, w);
            famKids[{h, w}].push_back(p.id);
            marriages.insert({h, w});
        } else if (par.size() == 1) {
            famKids[{par[0], ""}].push_back(p.id);
            marriages.insert({par[0], ""});
        }
    }

    int famIdx = 1;
    for (const auto& pair : marriages) {
        f << "0 @F" << famIdx++ << "@ FAM\n";
        if (!pair.first.empty()) {
            auto* p1 = model->find(pair.first);
            f << "1 " << (p1 && p1->sex == "F" ? "WIFE" : "HUSB") << " @I_" << pair.first << "@\n";
        }
        if (!pair.second.empty()) {
            auto* p2 = model->find(pair.second);
            f << "1 " << (p2 && p2->sex == "M" ? "HUSB" : "WIFE") << " @I_" << pair.second << "@\n";
        }
        for (const auto& kidId : famKids[pair]) {
            f << "1 CHIL @I_" << kidId << "@\n";
        }
    }
    f << "0 TRLR\n";
    f.close();
    MessageBoxW(mainWindow, L"Экспорт в GEDCOM (.ged) успешно завершён!",
                L"GEDCOM 5.5.1", MB_OK | MB_ICONINFORMATION);
}

// ============================================================
// ОСНОВНАЯ ОТРИСОВКА
// ============================================================
static void paintTree(HDC dc) {
    RECT client; GetClientRect(canvas, &client);
    if (!client.right || !client.bottom) return;

    Bitmap buffer(client.right, client.bottom, PixelFormat32bppPARGB);
    Graphics g(&buffer);
    g.Clear(Ui::background());
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);

    g.TranslateTransform(panX, panY);
    g.ScaleTransform(zoom, zoom);
    renderTreeContent(g, zoom);

    g.ResetTransform();
    const REAL dpiScale = windowDpi(canvas) / 96.f;
    const REAL screenWidth = client.right / dpiScale;
    const REAL screenHeight = client.bottom / dpiScale;
    g.ScaleTransform(dpiScale, dpiScale);

    const REAL headerH = 54;
    const REAL footerH = 34;
    SolidBrush bar(Color(245, 20, 30, 46));
    g.FillRectangle(&bar, (REAL)0, (REAL)0, screenWidth, headerH);
    g.FillRectangle(&bar, (REAL)0, screenHeight - footerH, screenWidth, footerH);

    Ui::label(g, L"Семейная история", RectF(20, 15, 260, 28), 19, Ui::text(), true);
    std::wstring status = std::to_wstring(cards.size()) + L" / "
        + std::to_wstring(model->people.size()) + L" персон   ·   "
        + std::to_wstring((int)(zoom * 100)) + L"%";
    Ui::label(g, status, RectF(std::max(290.f, screenWidth - 260), 20, 250, 22), 12, Ui::muted());

    REAL legendX = 18;
    const REAL legendY = screenHeight - 23;
    auto legendItem = [&](Color color, const wchar_t* caption, REAL advance) {
        SolidBrush dot(color);
        g.FillEllipse(&dot, legendX, legendY + 3, 6.f, 6.f);
        Ui::label(g, caption, RectF(legendX + 13, legendY - 1, advance - 13, 20), 11, Ui::muted());
        legendX += advance;
    };
    legendItem(Ui::male(), L"Мужчины", 95);
    legendItem(Ui::female(), L"Женщины", 100);
    legendItem(Ui::gold(), L"Центр", 75);
    if (!highlightedPath.empty())
        legendItem(Ui::emerald(), L"Путь", 70);

    if (screenWidth > 760)
        Ui::label(g, L"Колёсико — масштаб   ·   Потянуть — обзор   ·   Правый клик — меню",
                  RectF(screenWidth - 420, legendY - 1, 405, 20), 11, Ui::muted());

    drawMinimap(g, screenWidth, screenHeight, client);

    if (!hoveredCard.empty() && !dragging) {
        if (auto* hp = model->find(hoveredCard)) {
            std::vector<std::string> par, spo, chi, sib;
            collectRelatives(hp->id, par, spo, chi, sib);
            std::wstring title = wide(hp->name);
            if (!hp->maiden.empty()) title += L" (урожд. " + wide(hp->maiden) + L")";
            std::wstring line2 = Ui::branchName(hp->branch) + L"   ·   Поколение " + std::to_wstring(hp->gen);
            if (!hp->dates.empty()) line2 += L"   ·   " + wide(hp->dates);
            std::wstring line3 = L"Родителей: " + std::to_wstring(par.size())
                               + L"   Детей: " + std::to_wstring(chi.size())
                               + L"   Супруг(а): " + std::to_wstring(spo.size());
            const REAL tipW = 380.f, tipH = 84.f;
            const REAL mx = mouseCanvasPt.x / dpiScale;
            const REAL my = mouseCanvasPt.y / dpiScale;
            REAL tipX = mx + 18.f, tipY = my + 18.f;
            if (tipX + tipW > screenWidth - 12.f) tipX = mx - tipW - 12.f;
            if (tipY + tipH > screenHeight - 46.f) tipY = my - tipH - 12.f;
            RectF tipRect(tipX, tipY, tipW, tipH);
            GraphicsPath tipPath;
            Ui::rounded(tipPath, tipRect, 10);
            SolidBrush tipBg(Color(248, 16, 24, 38));
            Pen tipBorder(Ui::accent(*hp), 1.4f);
            g.FillPath(&tipBg, &tipPath);
            g.DrawPath(&tipBorder, &tipPath);
            Ui::label(g, title, RectF(tipX + 14, tipY + 10, tipW - 28, 22), 14, Ui::text(), true);
            Ui::label(g, line2, RectF(tipX + 14, tipY + 34, tipW - 28, 18), 11, Ui::muted());
            Ui::label(g, line3, RectF(tipX + 14, tipY + 56, tipW - 28, 18), 11, Ui::accent(*hp));
        }
    }

    g.ResetTransform();
    Graphics screen(dc);
    screen.DrawImage(&buffer, 0, 0);
}

// ============================================================
// ПРАВАЯ ПАНЕЛЬ
// ============================================================
static LRESULT CALLBACK DetailsProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_ERASEBKGND: return 1;

    case WM_MOUSEWHEEL: {
        RECT rc; GetClientRect(h, &rc);
        int step = (int)((48 * (int)windowDpi(h)) / 96);
        detailsScroll -= GET_WHEEL_DELTA_WPARAM(w) / WHEEL_DELTA * step;
        int maxScroll = std::max(0, detailsContentHeight - (int)rc.bottom);
        detailsScroll = std::clamp(detailsScroll, 0, maxScroll);
        InvalidateRect(h, nullptr, FALSE);
        return 0;
    }

    case WM_MOUSEMOVE: {
        POINT pt = { GET_X_LPARAM(l), GET_Y_LPARAM(l) };
        const REAL dpiScale = (REAL)windowDpi(h) / 96.f;
        float cx = pt.x / dpiScale, cy = (pt.y + detailsScroll) / dpiScale;
        std::string newHover;
        for (const auto& hit : detailHits)
            if (hit.rect.Contains(PointF(cx, cy))) { newHover = hit.id; break; }
        if (newHover != hoveredDetail) { hoveredDetail = newHover; InvalidateRect(h, nullptr, FALSE); }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        POINT pt = { GET_X_LPARAM(l), GET_Y_LPARAM(l) };
        const REAL dpiScale = (REAL)windowDpi(h) / 96.f;
        float cx = pt.x / dpiScale, cy = (pt.y + detailsScroll) / dpiScale;
        for (const auto& hit : detailHits)
            if (hit.rect.Contains(PointF(cx, cy))) {
                auto* target = model->find(hit.id);
                if (target && !matchesBranchFilter(*target) && !currentBranchFilter.empty()) {
                    applyBranchFilter("");
                }
                selectPerson(hit.id, true);
                return 0;
            }
        return 0;
    }

    case WM_SETCURSOR: {
        POINT pt; GetCursorPos(&pt); ScreenToClient(h, &pt);
        const REAL dpiScale = (REAL)windowDpi(h) / 96.f;
        float cx = pt.x / dpiScale, cy = (pt.y + detailsScroll) / dpiScale;
        bool over = false;
        for (const auto& hit : detailHits)
            if (hit.rect.Contains(PointF(cx, cy))) { over = true; break; }
        SetCursor(LoadCursorW(nullptr, over ? IDC_HAND : IDC_ARROW));
        return TRUE;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        if (!rc.right || !rc.bottom) { EndPaint(h, &ps); return 0; }

        Bitmap buffer(rc.right, rc.bottom, PixelFormat32bppPARGB);
        Graphics g(&buffer);
        g.Clear(Ui::panel());
        g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
        g.SetSmoothingMode(SmoothingModeAntiAlias);

        detailHits.clear();
        const REAL dpiScale = (REAL)windowDpi(h) / 96.f;
        g.TranslateTransform(0, (REAL)-detailsScroll);
        g.ScaleTransform(dpiScale, dpiScale);

        const REAL width = rc.right / dpiScale;
        const REAL x = 20;
        const REAL contentWidth = width - 40;
        REAL y = 22;

        auto* p = model->find(selected);
        auto textBlock = [&](const std::wstring& value, REAL size, Color color,
                             bool bold, bool clickable = false, const std::string& clickId = "") {
            if (value.empty()) return;
            REAL th = Ui::textHeight(g, value, contentWidth, size, bold);
            RectF rect(x, y, contentWidth, th + 4);
            if (clickable) {
                const bool isHover = (!clickId.empty() && clickId == hoveredDetail);
                SolidBrush hl(Color(isHover ? 36 : 16, 255, 255, 255));
                g.FillRectangle(&hl, rect);
                if (isHover) {
                    Pen underline(Ui::gold(), 1.2f);
                    g.DrawLine(&underline, rect.X + 14, rect.GetBottom() - 3,
                                            rect.GetRight() - 8, rect.GetBottom() - 3);
                }
            }
            Ui::label(g, value, rect, size, color, bold, true);
            if (clickable && !clickId.empty()) detailHits.push_back({ clickId, rect });
            y += th + 8;
        };

        auto section = [&](const std::wstring& title) {
            y += 18;
            Pen sep(Color(60, 163, 180, 204), 1);
            g.DrawLine(&sep, x, y, x + contentWidth, y);
            y += 14;
            textBlock(title, 11, Ui::muted(), true);
        };

        if (!p) {
            // ---- ДАШБОРД АНАЛИТИКИ ----
            textBlock(L"Хроника и аналитика", 21, Ui::gold(), true);
            textBlock(L"Кликните на рекордсмена ниже для перехода к его карточке.", 12, Ui::muted(), false);

            int males = 0, females = 0, marriagesCnt = 0;
            int sumLifespan = 0, countLifespan = 0;
            struct RecordPerson { std::string id; std::wstring label; int val; };
            std::vector<RecordPerson> longevityList, childrenList;

            for (const auto& person : model->people) {
                if (person.sex == "M") ++males;
                else if (person.sex == "F") ++females;

                auto lf = parseDates(person.dates);
                if (lf.age > 0) {
                    if (!lf.isAlive) { sumLifespan += lf.age; ++countLifespan; }
                    std::wstring tag = wide(person.name) + L" — " + std::to_wstring(lf.age) + L" лет";
                    longevityList.push_back({ person.id, tag, lf.age });
                }

                std::vector<std::string> par, spo, chi, sib;
                collectRelatives(person.id, par, spo, chi, sib);
                if (!chi.empty()) {
                    std::wstring tag = wide(person.name) + L" — " + std::to_wstring(chi.size()) + L" дет.";
                    childrenList.push_back({ person.id, tag, (int)chi.size() });
                }
            }
            for (const auto& r : model->relations) if (r.type == "marriage") ++marriagesCnt;

            std::sort(longevityList.begin(), longevityList.end(), [](const auto& a, const auto& b) { return a.val > b.val; });
            std::sort(childrenList.begin(), childrenList.end(), [](const auto& a, const auto& b) { return a.val > b.val; });

            section(L"ОБЩАЯ СВОДКА РОДА");
            textBlock(L"Всего в древе: " + std::to_wstring(model->people.size()) + L" чел.", 13, Ui::text(), false);
            textBlock(L"Мужчин: " + std::to_wstring(males) + L"   ·   Женщин: " + std::to_wstring(females), 13, Ui::text(), false);
            textBlock(L"Семейных союзов (браков): " + std::to_wstring(marriagesCnt), 13, Ui::text(), false);
            if (countLifespan > 0)
                textBlock(L"Средняя прод. жизни: " + std::to_wstring(sumLifespan / countLifespan) + L" лет", 13, Ui::gold(), true);

            if (!longevityList.empty()) {
                section(L"СТАРШИЕ И ДОЛГОЖИТЕЛИ");
                for (size_t i = 0; i < std::min<size_t>(4, longevityList.size()); ++i)
                    textBlock(L"• " + longevityList[i].label, 14, Ui::text(), false, true, longevityList[i].id);
            }

            if (!childrenList.empty()) {
                section(L"САМЫЕ БОЛЬШИЕ СЕМЬИ");
                for (size_t i = 0; i < std::min<size_t>(4, childrenList.size()); ++i)
                    textBlock(L"• " + childrenList[i].label, 14, Ui::text(), false, true, childrenList[i].id);
            }

            detailsContentHeight = (int)((y + 24) * dpiScale);
        } else {
            textBlock(wide(p->name), 24, Ui::accent(*p), true);
            if (!p->maiden.empty()) textBlock(L"урожд. " + wide(p->maiden), 13, Ui::muted(), false);
            if (!p->dates.empty())  textBlock(wide(p->dates), 14, Ui::text(), false);
            textBlock(Ui::branchName(p->branch), 13, Ui::muted(), false);

            std::vector<std::string> parents, spouses, children, siblings;
            collectRelatives(p->id, parents, spouses, children, siblings);

            auto relatives = [&](const std::wstring& title, const std::vector<std::string>& ids) {
                if (ids.empty()) return;
                section(title + L"  ·  " + std::to_wstring(ids.size()));
                for (const auto& rid : ids)
                    if (auto* r = model->find(rid))
                        textBlock(L"• " + wide(r->name), 15, Ui::text(), false, true, rid);
            };
            relatives(L"РОДИТЕЛИ", parents);
            relatives(L"СУПРУГИ", spouses);
            relatives(L"ДЕТИ", children);
            relatives(L"БРАТЬЯ И СЁСТРЫ", siblings);

            detailsContentHeight = (int)((y + 24) * dpiScale);
        }

        Graphics screen(dc);
        screen.DrawImage(&buffer, 0, 0);
        EndPaint(h, &ps);
        return 0;
    }
    }
    return DefWindowProcW(h, msg, w, l);
}

// ============================================================
// КНОПКИ
// ============================================================
struct BtnState {
    Color accent;
    bool hovered = false;
    bool pressed = false;
};
static std::map<HWND, BtnState> btnStates;

static LRESULT CALLBACK ButtonProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    auto& state = btnStates[h];
    switch (msg) {
    case WM_ERASEBKGND: return 1;
    case WM_SETCURSOR:
        SetCursor(LoadCursorW(nullptr, IDC_HAND));
        return TRUE;
    case WM_MOUSEMOVE:
        if (!state.hovered) {
            state.hovered = true;
            TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, h, 0 };
            TrackMouseEvent(&tme);
            InvalidateRect(h, nullptr, FALSE);
        }
        return 0;
    case WM_MOUSELEAVE:
        state.hovered = false;
        state.pressed = false;
        InvalidateRect(h, nullptr, FALSE);
        return 0;
    case WM_LBUTTONDOWN:
        state.pressed = true;
        SetCapture(h);
        InvalidateRect(h, nullptr, FALSE);
        return 0;
    case WM_LBUTTONUP: {
        bool wasPressed = state.pressed;
        state.pressed = false;
        POINT pt = { GET_X_LPARAM(l), GET_Y_LPARAM(l) };
        RECT rc; GetClientRect(h, &rc);
        bool inside = (pt.x >= 0 && pt.y >= 0 && pt.x < rc.right && pt.y < rc.bottom);
        if (GetCapture() == h) ReleaseCapture();
        InvalidateRect(h, nullptr, FALSE);
        if (wasPressed && inside) {
            int id = GetDlgCtrlID(h);
            PostMessageW(GetParent(h), WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), (LPARAM)h);
        }
        return 0;
    }
    case WM_CAPTURECHANGED: return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        if (!rc.right || !rc.bottom) { EndPaint(h, &ps); return 0; }

        Bitmap buffer(rc.right, rc.bottom, PixelFormat32bppPARGB);
        Graphics g(&buffer);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);

        const REAL w = (REAL)rc.right, hgt = (REAL)rc.bottom, pad = 1.5f;
        RectF r(pad, pad, w - pad * 2, hgt - pad * 2);

        Color topColor, botColor;
        if (state.pressed) { topColor = Color(255, 20, 28, 44); botColor = Color(255, 26, 36, 52); }
        else if (state.hovered) { topColor = Color(255, 40, 56, 82); botColor = Color(255, 32, 46, 68); }
        else { topColor = Color(255, 32, 46, 68); botColor = Color(255, 26, 38, 58); }

        GraphicsPath shape;
        Ui::rounded(shape, r, 8);
        LinearGradientBrush fill(PointF(r.X, r.Y), PointF(r.X, r.GetBottom()), topColor, botColor);
        g.FillPath(&fill, &shape);

        Color borderColor = (state.pressed || state.hovered) ? state.accent
                          : Color(state.accent.GetR(), state.accent.GetG(), state.accent.GetB(), 120);
        Pen border(borderColor, state.pressed ? 2.0f : 1.2f);
        g.DrawPath(&border, &shape);

        wchar_t btnText[128];
        GetWindowTextW(h, btnText, 128);
        Gdiplus::Font font(L"Segoe UI", 13, FontStyleBold, UnitPixel);
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);
        SolidBrush textBrush(state.hovered || state.pressed ? Ui::text() : Color(255, 210, 222, 240));
        g.DrawString(btnText, -1, &font, r, &format, &textBrush);

        Graphics screen(dc);
        screen.DrawImage(&buffer, 0, 0);
        EndPaint(h, &ps);
        return 0;
    }
    }
    return DefWindowProcW(h, msg, w, l);
}

static HWND makeButton(HWND parent, int id, const wchar_t* text, Color accent) {
    HWND btn = CreateWindowExW(0, L"FamilyTreeButton", text,
        WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, parent, (HMENU)(INT_PTR)id, nullptr, nullptr);
    btnStates[btn] = BtnState{ accent };
    return btn;
}

// ============================================================
// CANVAS
// ============================================================
static void copyToClipboard(HWND owner, const std::wstring& str) {
    if (!OpenClipboard(owner)) return;
    EmptyClipboard();
    size_t bytes = (str.size() + 1) * sizeof(wchar_t);
    if (HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes)) {
        if (void* p = GlobalLock(hMem)) {
            memcpy(p, str.c_str(), bytes);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
    }
    CloseClipboard();
}

static LRESULT CALLBACK CanvasProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
        case WM_ERASEBKGND: return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
            paintTree(dc); EndPaint(h, &ps); return 0;
        }
        case WM_LBUTTONDOWN: {
            POINT pt = { GET_X_LPARAM(l), GET_Y_LPARAM(l) };
            RECT client; GetClientRect(h, &client);
            const REAL dpiScale = windowDpi(h) / 96.f;
            RectF mm = getMinimapRect(client.right / dpiScale, client.bottom / dpiScale);
            PointF sp(pt.x / dpiScale, pt.y / dpiScale);
            if (mm.Contains(sp)) {
                const REAL pad = 8.f;
                const REAL scale = std::min((mm.Width - pad * 2) / std::max(1.f, treeWidth),
                                            (mm.Height - pad * 2) / std::max(1.f, treeHeight));
                const REAL ox = mm.X + (mm.Width - treeWidth * scale) / 2.f;
                const REAL oy = mm.Y + (mm.Height - treeHeight * scale) / 2.f;
                panX = client.right / 2.f - ((sp.X - ox) / scale) * zoom;
                panY = client.bottom / 2.f - ((sp.Y - oy) / scale) * zoom;
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
            mouseStart = mouseLast = pt;
            dragging = true; moved = false; SetCapture(h); return 0;
        }
        case WM_MOUSEMOVE: {
            mouseCanvasPt = { GET_X_LPARAM(l), GET_Y_LPARAM(l) };
            if (dragging) {
                if (std::abs(mouseCanvasPt.x - mouseStart.x) + std::abs(mouseCanvasPt.y - mouseStart.y) > 4) moved = true;
                panX += mouseCanvasPt.x - mouseLast.x;
                panY += mouseCanvasPt.y - mouseLast.y;
                mouseLast = mouseCanvasPt;
                InvalidateRect(h, nullptr, FALSE);
            } else {
                PointF wp((mouseCanvasPt.x - panX) / zoom, (mouseCanvasPt.y - panY) / zoom);
                std::string newHover;
                for (const auto& card : cards)
                    if (card.rect.Contains(wp)) { newHover = card.id; break; }
                if (newHover != hoveredCard) {
                    hoveredCard = newHover;
                    SetCursor(LoadCursorW(nullptr, hoveredCard.empty() ? IDC_ARROW : IDC_HAND));
                    InvalidateRect(h, nullptr, FALSE);
                }
            }
            TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, h, 0 };
            TrackMouseEvent(&tme);
            return 0;
        }
        case WM_MOUSELEAVE:
            if (!hoveredCard.empty()) {
                hoveredCard.clear();
                SetCursor(LoadCursorW(nullptr, IDC_ARROW));
                InvalidateRect(h, nullptr, FALSE);
            }
            return 0;
        case WM_CAPTURECHANGED: dragging = false; return 0;
        case WM_LBUTTONUP: {
            bool click = dragging && !moved; dragging = false; ReleaseCapture();
            if (click) {
                PointF p((GET_X_LPARAM(l) - panX) / zoom, (GET_Y_LPARAM(l) - panY) / zoom);
                for (auto& card : cards) if (card.rect.Contains(p)) {
                    selectPerson(card.id, false); break;
                }
            }
            return 0;
        }
        case WM_MOUSEWHEEL: {
            POINT p = { GET_X_LPARAM(l), GET_Y_LPARAM(l) };
            ScreenToClient(h, &p);
            float next = std::clamp(zoom * (GET_WHEEL_DELTA_WPARAM(w) > 0 ? 1.15f : 1 / 1.15f), .025f, 3.f);
            panX = p.x - (p.x - panX) * next / zoom;
            panY = p.y - (p.y - panY) * next / zoom;
            zoom = next;
            InvalidateRect(h, nullptr, FALSE); return 0;
        }
        case WM_RBUTTONUP: {
            PointF wp((GET_X_LPARAM(l) - panX) / zoom, (GET_Y_LPARAM(l) - panY) / zoom);
            std::string targetId;
            for (const auto& card : cards)
                if (card.rect.Contains(wp)) { targetId = card.id; break; }
            if (targetId.empty()) return 0;
            auto* p = model->find(targetId);
            if (!p) return 0;

            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING, 1, L"Центрировать в дереве");
            AppendMenuW(menu, MF_STRING, 2, L"Скопировать имя");
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

            if (!selected.empty() && selected != targetId) {
                if (auto* sp = model->find(selected)) {
                    std::wstring item = L"Путь родства: до " + wide(sp->name);
                    AppendMenuW(menu, MF_STRING, 3, item.c_str());
                }
            }
            if (targetId != "venera" && model->find("venera"))
                AppendMenuW(menu, MF_STRING, 4, L"Путь до Венеры");
            if (!highlightedPath.empty())
                AppendMenuW(menu, MF_STRING, 5, L"Сбросить подсветку пути");

            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(menu, MF_STRING, 6, L"Экспорт PNG");
            AppendMenuW(menu, MF_STRING, 7, L"Экспорт в GEDCOM (.ged)...");

            POINT sp = { GET_X_LPARAM(l), GET_Y_LPARAM(l) };
            ClientToScreen(h, &sp);
            int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, sp.x, sp.y, 0, h, nullptr);
            DestroyMenu(menu);

            if (cmd == 1) selectPerson(targetId, true);
            else if (cmd == 2) copyToClipboard(mainWindow, wide(p->name));
            else if (cmd == 3) { highlightedPath = computePath(selected, targetId); InvalidateRect(h, nullptr, FALSE); }
            else if (cmd == 4) { highlightedPath = computePath(targetId, "venera"); InvalidateRect(h, nullptr, FALSE); }
            else if (cmd == 5) { highlightedPath.clear(); InvalidateRect(h, nullptr, FALSE); }
            else if (cmd == 6) exportPng();
            else if (cmd == 7) exportGedcom();
            return 0;
        }
    }
    return DefWindowProcW(h, msg, w, l);
}

static void resize(HWND h) {
    RECT r; GetClientRect(h, &r);
    int width = r.right, height = r.bottom;
    MoveWindow(searchBox, 16, 62, 238, 32, TRUE);
    MoveWindow(peopleList, 16, 106, 238, std::max(50, height - 156), TRUE);
    MoveWindow(detailBox, std::max(280, width - 286), 62, 270, std::max(50, height - 114), TRUE);
    MoveWindow(canvas, 270, 54, std::max(50, width - 570), std::max(50, height - 104), TRUE);
    MoveWindow(GetDlgItem(h, 102), 16, 14, 120, 34, TRUE);
    MoveWindow(GetDlgItem(h, 103), 144, 14, 110, 34, TRUE);
    MoveWindow(GetDlgItem(h, 105), 262, 14, 80, 34, TRUE);
    MoveWindow(GetDlgItem(h, 106), 350, 14, 180, 34, TRUE);
}

static LRESULT CALLBACK MainProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
        case WM_CREATE:
            mainWindow = h;
            searchBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 0, 0, h, (HMENU)100, nullptr, nullptr);
            SendMessageW(searchBox, 0x1501, TRUE, (LPARAM)L"Поиск родственника...");
            peopleList = CreateWindowExW(0, L"LISTBOX", L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT
                | LBS_OWNERDRAWVARIABLE | LBS_HASSTRINGS, 0, 0, 0, 0, h, (HMENU)101, nullptr, nullptr);
            detailBox = CreateWindowExW(0, L"FamilyTreeDetails", L"",
                WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 0, 0, h, (HMENU)104, nullptr, nullptr);

            makeButton(h, 102, L"Показать всё", Ui::male());
            makeButton(h, 103, L"К Венере", Ui::gold());
            makeButton(h, 105, L"PNG", Ui::green());
            makeButton(h, 106, L"Ветвь: Все  ▾", Ui::male());

            canvas = CreateWindowExW(0, L"FamilyTreeCanvas", L"", WS_CHILD | WS_VISIBLE,
                0, 0, 0, 0, h, nullptr, nullptr, nullptr);
            SendMessageW(searchBox, WM_SETFONT, (WPARAM)uiFont, TRUE);
            SendMessageW(peopleList, WM_SETFONT, (WPARAM)uiFont, TRUE);
            arrange(); filterList(); return 0;

        case WM_SIZE: resize(h); return 0;
        case WM_GETMINMAXINFO: ((MINMAXINFO*)l)->ptMinTrackSize = { 1000, 650 }; return 0;

        case WM_MEASUREITEM: {
            auto* m = reinterpret_cast<MEASUREITEMSTRUCT*>(l);
            if (m->CtlID == 101) {
                int dpi = (int)windowDpi(h);
                int idx = (int)m->itemID;
                bool isHeader = (idx >= 0 && (size_t)idx < listIds.size() && listIds[idx].empty());
                m->itemHeight = MulDiv(isHeader ? 32 : 54, dpi, 96);
                return TRUE;
            }
            break;
        }

        case WM_DRAWITEM: {
            auto* d = reinterpret_cast<DRAWITEMSTRUCT*>(l);
            if (d->CtlID != 101) break;
            if ((int)d->itemID == -1 || d->itemID >= listIds.size()) return TRUE;

            const REAL scale = (REAL)windowDpi(peopleList) / 96.f;
            Graphics g(d->hDC);
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
            const RectF r((REAL)d->rcItem.left, (REAL)d->rcItem.top,
                          (REAL)(d->rcItem.right - d->rcItem.left),
                          (REAL)(d->rcItem.bottom - d->rcItem.top));

            if (listIds[d->itemID].empty()) {
                SolidBrush bg(Ui::background());
                g.FillRectangle(&bg, r);
                wchar_t buf[256];
                SendMessageW(peopleList, LB_GETTEXT, d->itemID, (LPARAM)buf);
                Ui::label(g, buf, RectF(r.X + 10, r.Y + 8, r.Width - 20, 20), 10, Ui::muted(), true);
                return TRUE;
            }

            auto* person = model->find(listIds[d->itemID]);
            if (!person) return TRUE;
            const bool active = (d->itemState & ODS_SELECTED) != 0;
            SolidBrush bg(active ? Color(255, 43, 62, 88) : Ui::panel());
            g.FillRectangle(&bg, r);
            SolidBrush dot(Ui::accent(*person));
            g.FillEllipse(&dot, r.X + 12 * scale, r.Y + 14 * scale, 6 * scale, 6 * scale);
            Ui::label(g, wide(person->name),
                      RectF(r.X + 28 * scale, r.Y + 7 * scale, r.Width - 36 * scale, 23 * scale),
                      14 * scale, Ui::text(), active);
            Ui::label(g, Ui::branchName(person->branch),
                      RectF(r.X + 28 * scale, r.Y + 31 * scale, r.Width - 36 * scale, 17 * scale),
                      11 * scale, Ui::muted());
            if (d->itemState & ODS_FOCUS) DrawFocusRect(d->hDC, &d->rcItem);
            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(w) == 100 && HIWORD(w) == EN_CHANGE) filterList();
            if (LOWORD(w) == 101 && HIWORD(w) == LBN_SELCHANGE) {
                auto index = SendMessageW(peopleList, LB_GETCURSEL, 0, 0);
                if (index >= 0 && (size_t)index < listIds.size()) {
                    if (listIds[index].empty()) {
                        int prevIdx = -1;
                        for (size_t i = 0; i < listIds.size(); ++i)
                            if (!listIds[i].empty() && listIds[i] == selected) { prevIdx = (int)i; break; }
                        SendMessageW(peopleList, LB_SETCURSEL, (WPARAM)prevIdx, 0);
                    } else {
                        selectPerson(listIds[index], true);
                    }
                }
            }
            if (LOWORD(w) == 102) {
                if (!currentBranchFilter.empty()) applyBranchFilter("");
                else fit();
            }
            if (LOWORD(w) == 103 && model->find("venera")) {
                if (currentBranchFilter != "" && currentBranchFilter != "center")
                    applyBranchFilter("");
                selectPerson("venera", true);
            }
            if (LOWORD(w) == 105) exportPng();
            if (LOWORD(w) == 106) {
                HMENU menu = CreatePopupMenu();
                AppendMenuW(menu, MF_STRING | (currentBranchFilter.empty() ? MF_CHECKED : 0), 1, L"Все ветви");
                AppendMenuW(menu, MF_STRING | (currentBranchFilter == "paternal" ? MF_CHECKED : 0), 2, L"Только отцовская");
                AppendMenuW(menu, MF_STRING | (currentBranchFilter == "maternal" ? MF_CHECKED : 0), 3, L"Только материнская");
                AppendMenuW(menu, MF_STRING | (currentBranchFilter == "center" ? MF_CHECKED : 0), 4, L"Только центральная");
                POINT pt; GetCursorPos(&pt);
                int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, h, nullptr);
                DestroyMenu(menu);
                if (cmd == 1) applyBranchFilter("");
                else if (cmd == 2) applyBranchFilter("paternal");
                else if (cmd == 3) applyBranchFilter("maternal");
                else if (cmd == 4) applyBranchFilter("center");
            }
            return 0;

        case WM_CTLCOLOREDIT:
            SetTextColor((HDC)w, RGB(218, 229, 246));
            SetBkColor((HDC)w, RGB(30, 44, 66));
            return (LRESULT)panelBrush;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORLISTBOX:
            SetTextColor((HDC)w, RGB(218, 229, 246));
            SetBkColor((HDC)w, RGB(23, 34, 51));
            return (LRESULT)panelBrush;

        case WM_PAINT: {
            PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
            RECT r; GetClientRect(h, &r);
            FillRect(dc, &r, panelBrush);
            EndPaint(h, &ps);
            return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, msg, w, l);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    SetProcessDPIAware();
    ULONG_PTR token;
    GdiplusStartupInput startup;
    if (GdiplusStartup(&token, &startup, nullptr) != Ok) return 1;

    int result = 0;
    try {
        if (!fs::exists(exe_dir() / "data" / "family_data.json"))
            throw std::runtime_error("Нет data/family_data.json рядом с FamilyTree.exe.");
        model = std::make_unique<FamilyTree>();
        uiFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        panelBrush = CreateSolidBrush(RGB(23, 34, 51));

        WNDCLASSW c{};
        c.hInstance = instance;
        c.lpfnWndProc = CanvasProc;
        c.lpszClassName = L"FamilyTreeCanvas";
        c.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        if (!RegisterClassW(&c)) throw std::runtime_error("Canvas class.");

        c.lpfnWndProc = DetailsProc;
        c.lpszClassName = L"FamilyTreeDetails";
        c.hbrBackground = nullptr;
        if (!RegisterClassW(&c)) throw std::runtime_error("Details class.");

        c.lpfnWndProc = ButtonProc;
        c.lpszClassName = L"FamilyTreeButton";
        c.hbrBackground = nullptr;
        if (!RegisterClassW(&c)) throw std::runtime_error("Button class.");

        c.lpfnWndProc = MainProc;
        c.lpszClassName = L"FamilyTreeMain";
        c.hbrBackground = panelBrush;
        c.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APPICON));
	if (!c.hIcon) c.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        if (!RegisterClassW(&c)) throw std::runtime_error("Main class.");

        HWND window = CreateWindowExW(0, c.lpszClassName, L"FamilyTree — семейная история",
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
            1450, 900, nullptr, nullptr, instance, nullptr);
        if (!window) throw std::runtime_error("Не удалось открыть окно.");
        ShowWindow(window, show); UpdateWindow(window); fit();

        MSG message;
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            if (message.message == WM_KEYDOWN) {
                bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                if (ctrl && message.wParam == 'F') { SetFocus(searchBox); SendMessageW(searchBox, EM_SETSEL, 0, -1); continue; }
                if (ctrl && message.wParam == 'E') { exportPng(); continue; }
                if (ctrl && message.wParam == 'G') { exportGedcom(); continue; }
                if (ctrl && (message.wParam == '0' || message.wParam == VK_NUMPAD0)) { fit(); continue; }
                if (message.wParam == VK_ESCAPE) {
                    selected.clear();
                    highlightedPath.clear();
                    SetWindowTextW(searchBox, L"");
                    if (!currentBranchFilter.empty()) applyBranchFilter("");
                    else filterList();
                    InvalidateRect(canvas, nullptr, FALSE);
                    InvalidateRect(detailBox, nullptr, FALSE);
                    continue;
                }
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    } catch (const std::exception& e) {
        MessageBoxW(nullptr, wide(e.what()).c_str(), L"FamilyTree — ошибка", MB_OK | MB_ICONERROR);
        result = 1;
    }

    model.reset();
    if (uiFont) DeleteObject(uiFont);
    if (panelBrush) DeleteObject(panelBrush);
    GdiplusShutdown(token);
    return result;
}