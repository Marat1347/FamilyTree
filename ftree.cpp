// ftree.cpp — генеалогическое дерево, CLI, C++17
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <limits>
#include <iomanip>
#include <cmath>
#include <cstdlib>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace fs = std::filesystem;

#define RST "\033[0m"
#define BLD "\033[1m"
#define DIM "\033[2m"
#define RED "\033[91m"
#define GRN "\033[92m"
#define YLW "\033[93m"
#define MAG "\033[95m"
#define CYN "\033[96m"

static void setup_console() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD m = 0;
    if (GetConsoleMode(h, &m))
        SetConsoleMode(h, m | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

static fs::path exe_dir() {
#ifdef _WIN32
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return fs::path(buf).parent_path();
#else
    return fs::current_path();
#endif
}


// ==================== ОФОРМЛЕНИЕ ====================
#define WHT "\033[97m"

static std::string rep(const std::string& s, int n) {
    std::string r;
    for (int i = 0; i < n; ++i) r += s;
    return r;
}

// Ширина для обычной кириллицы и выбранных одиночных символов.
// Это не универсальная реализация Unicode grapheme/wcwidth.
static size_t vis_len(const std::string& s) {
    size_t n = 0;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c == 27 && i + 1 < s.size() && s[i + 1] == '[') {
            i += 2;
            while (i < s.size()) {
                unsigned char end = static_cast<unsigned char>(s[i++]);
                if (end >= 0x40 && end <= 0x7e) break;
            }
        } else {
            if ((c & 0xc0) != 0x80 && c >= 32) ++n;
            ++i;
        }
    }
    return n;
}
static std::string pad_to(const std::string& s, size_t width) {
    const size_t n = vis_len(s);
    return s + std::string(n < width ? width - n : 0, ' ');
}
static std::string gen_color(int g) {
    static const char* colors[] = {
        "\033[93m", "\033[38;5;208m", "\033[95m", "\033[96m",
        "\033[92m", "\033[37m", "\033[90m"
    };
    return colors[((g % 7) + 7) % 7];
}
static std::string sex_icon(const std::string& s) {
    return s == "M" ? "\033[96m♂\033[0m"
         : s == "F" ? "\033[95m♀\033[0m" : "\033[90m·\033[0m";
}
static std::string branch_color(const std::string& b) {
    return b == "paternal" ? "\033[94m" : b == "maternal" ? "\033[92m"
         : b == "center" ? "\033[93m" : "\033[37m";
}
static std::string branch_mark(const std::string& b) {
    return b == "paternal" ? "▓" : b == "maternal" ? "▒"
         : b == "center" ? "◆" : "·";
}
static std::string styled(const std::string& text, const std::string& color) {
    return color + text + RST;
}
static void panel(const std::vector<std::string>& rows) {
    size_t width = 62;
    for (const auto& row : rows)
        if (row != "\x01") width = std::max(width, vis_len(row) + 2);
    std::cout << CYN << "╭" << rep("─", static_cast<int>(width)) << "╮\n" << RST;
    for (const auto& row : rows) {
        if (row == "\x01")
            std::cout << CYN << "├" << rep("─", static_cast<int>(width)) << "┤\n" << RST;
        else
            std::cout << CYN << "│ " << RST << pad_to(row, width - 2)
                      << CYN << " │\n" << RST;
    }
    std::cout << CYN << "╰" << rep("─", static_cast<int>(width)) << "╯\n" << RST;
}
static bool digits(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(),
        [](unsigned char c) { return c >= '0' && c <= '9'; });
}
static int generation_number(const std::string& s) {
    if (!digits(s)) throw std::runtime_error("Поколение должно быть целым от 0 до 1000.");
    const long n = std::stol(s);
    if (n > 1000) throw std::runtime_error("Поколение должно быть от 0 до 1000.");
    return static_cast<int>(n);
}
// Кавычки позволяют передавать пути с пробелами. Обратные слеши сохраняются.
static std::vector<std::string> split_command(const std::string& line) {
    std::vector<std::string> args;
    std::string token;
    bool quoted = false, started = false;
    for (char c : line) {
        if (c == '"') { quoted = !quoted; started = true; }
        else if (!quoted && std::isspace(static_cast<unsigned char>(c))) {
            if (started) { args.push_back(token); token.clear(); started = false; }
        } else { token += c; started = true; }
    }
    if (quoted) throw std::runtime_error("Не закрыта кавычка в команде.");
    if (started) args.push_back(token);
    return args;
}

struct JVal;
using JObj = std::map<std::string, JVal>;
using JArr = std::vector<JVal>;

struct JVal {
    enum T { NUL, BOOL, NUM, STR, ARR, OBJ } t = NUL;
    double n = 0;
    std::string s;
    JArr a;
    JObj o;
    bool has(const std::string& k) const { return t == OBJ && o.count(k); }
    const JVal& at(const std::string& k) const { return o.at(k); }
    std::string str(const std::string& k) const {
        return has(k) && at(k).t == STR ? at(k).s : std::string();
    }
    int num(const std::string& k, int def = 0) const {
        return has(k) && at(k).t == NUM ? (int)at(k).n : def;
    }
};

class JParser {
    std::string src;
    size_t pos = 0;
    unsigned depth = 0;
    [[noreturn]] void fail(const std::string& message) const {
        throw std::runtime_error("JSON, байт " + std::to_string(pos) + ": " + message);
    }
    void ws() {
        while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\r' ||
               src[pos] == '\n' || src[pos] == '\t')) ++pos;
    }
    char peek() { ws(); return pos < src.size() ? src[pos] : '\0'; }
    void expect(char c) {
        if (peek() != c) fail(std::string("ожидается ") + c);
        ++pos;
    }
    unsigned hex4() {
        if (pos + 4 > src.size()) fail("неполная Unicode-последовательность");
        unsigned n = 0;
        for (int i = 0; i < 4; ++i) {
            char c = src[pos++];
            int digit = c >= '0' && c <= '9' ? c - '0'
                      : c >= 'a' && c <= 'f' ? c - 'a' + 10
                      : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
            if (digit < 0) fail("неверная Unicode-последовательность");
            n = n * 16 + static_cast<unsigned>(digit);
        }
        return n;
    }
    static void utf8(std::string& r, unsigned cp) {
        if (cp < 0x80) r += static_cast<char>(cp);
        else if (cp < 0x800) {
            r += static_cast<char>(0xc0 | (cp >> 6));
            r += static_cast<char>(0x80 | (cp & 63));
        } else if (cp < 0x10000) {
            r += static_cast<char>(0xe0 | (cp >> 12));
            r += static_cast<char>(0x80 | ((cp >> 6) & 63));
            r += static_cast<char>(0x80 | (cp & 63));
        } else {
            r += static_cast<char>(0xf0 | (cp >> 18));
            r += static_cast<char>(0x80 | ((cp >> 12) & 63));
            r += static_cast<char>(0x80 | ((cp >> 6) & 63));
            r += static_cast<char>(0x80 | (cp & 63));
        }
    }
    std::string str() {
        expect('"'); std::string r;
        while (pos < src.size()) {
            unsigned char c = static_cast<unsigned char>(src[pos++]);
            if (c == '"') return r;
            if (c < 32) fail("управляющий символ в строке");
            if (c != '\\') { r += static_cast<char>(c); continue; }
            if (pos == src.size()) fail("незавершённое экранирование");
            switch (src[pos++]) {
                case '"': r += '"'; break;
                case '\\': r += '\\'; break;
                case '/': r += '/'; break;
                case 'n': r += '\n'; break;
                case 'r': r += '\r'; break;
                case 't': r += '\t'; break;
                case 'b': r += '\b'; break;
                case 'f': r += '\f'; break;
                case 'u': {
                    unsigned cp = hex4();
                    if (cp >= 0xd800 && cp <= 0xdbff) {
                        if (src.compare(pos, 2, "\\u") != 0) fail("нет второй половины суррогатной пары");
                        pos += 2; unsigned low = hex4();
                        if (low < 0xdc00 || low > 0xdfff) fail("ошибочная суррогатная пара");
                        cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
                    } else if (cp >= 0xdc00 && cp <= 0xdfff) fail("одиночный суррогат");
                    utf8(r, cp); break;
                }
                default: fail("неизвестное экранирование");
            }
        }
        fail("строка не закрыта");
    }
    JVal val() {
        if (++depth > 128) fail("слишком большая вложенность");
        char c = peek(); JVal v;
        if (c == '{') {
            v.t = JVal::OBJ; ++pos;
            if (peek() != '}') while (true) {
                if (peek() != '"') fail("ключ должен быть строкой");
                std::string key = str(); expect(':');
                if (v.o.count(key)) fail("повторяющийся ключ");
                v.o.emplace(key, val());
                if (peek() != ',') break;
                ++pos;
            }
            expect('}');
        } else if (c == '[') {
            v.t = JVal::ARR; ++pos;
            if (peek() != ']') while (true) {
                v.a.push_back(val());
                if (peek() != ',') break;
                ++pos;
            }
            expect(']');
        } else if (c == '"') { v.t = JVal::STR; v.s = str(); }
        else if (src.compare(pos, 4, "true") == 0) { pos += 4; v.t = JVal::BOOL; v.n = 1; }
        else if (src.compare(pos, 5, "false") == 0) { pos += 5; v.t = JVal::BOOL; }
        else if (src.compare(pos, 4, "null") == 0) pos += 4;
        else {
            size_t start = pos;
            if (pos < src.size() && src[pos] == '-') ++pos;
            if (pos >= src.size()) fail("неполное число");
            if (src[pos] == '0') ++pos;
            else {
                if (src[pos] < '1' || src[pos] > '9') fail("ожидается значение");
                while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) ++pos;
            }
            auto decimal = [&]() {
                size_t begin = pos;
                while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) ++pos;
                if (begin == pos) fail("ожидаются цифры");
            };
            if (pos < src.size() && src[pos] == '.') { ++pos; decimal(); }
            if (pos < src.size() && (src[pos] == 'e' || src[pos] == 'E')) {
                ++pos;
                if (pos < src.size() && (src[pos] == '+' || src[pos] == '-')) ++pos;
                decimal();
            }
            v.t = JVal::NUM; v.n = std::stod(src.substr(start, pos - start));
            if (!std::isfinite(v.n)) fail("число вне диапазона");
        }
        --depth; return v;
    }
public:
    explicit JParser(std::string text) : src(std::move(text)) {
        if (src.compare(0, 3, "\xef\xbb\xbf") == 0) pos = 3;
    }
    JVal parse() {
        JVal result = val(); ws();
        if (pos != src.size()) fail("лишние символы после документа");
        return result;
    }
};

static std::string jesc(const std::string& s) {
    std::string r;
    for (char c : s) {
        if (c == '"') r += "\\\"";
        else if (c == '\\') r += "\\\\";
        else if (c == '\n') r += "\\n";
        else if (c == '\t') r += "\\t";
        else if (c == '\r') r += "\\r";
        else if (static_cast<unsigned char>(c) < 32) {
            const char* hex = "0123456789abcdef";
            unsigned char u = static_cast<unsigned char>(c);
            r += "\\u00"; r += hex[u >> 4]; r += hex[u & 15];
        } else r += c;
    }
    return r;
}

struct Person {
    std::string id, name, sex, branch, maiden, dates;
    int gen = 0;
};
struct Relation { std::string type, src, tgt; };

class FamilyTree {
public:
    std::vector<Person> people;
    std::vector<Relation> relations;
    fs::path path;
    std::vector<std::string> last_list;

    FamilyTree() {
        fs::path dir = exe_dir() / "data";
        std::error_code ec;
        fs::create_directories(dir, ec);
        path = dir / "family_data.json";
        if (fs::exists(path)) load();
        else { seed_default(); save(); }
    }


    void validate_data(const std::vector<Person>& ps, const std::vector<Relation>& rs) {
        std::map<std::string, const Person*> index;
        for (const auto& p : ps) {
            if (p.id.empty() || p.name.empty()) throw std::runtime_error("Пустой ID или имя.");
            if (!index.emplace(p.id, &p).second) throw std::runtime_error("Повтор ID: " + p.id);
            if (p.gen < 0 || p.gen > 1000) throw std::runtime_error("Поколение вне диапазона: " + p.id);
            if (!p.sex.empty() && p.sex != "M" && p.sex != "F") throw std::runtime_error("Пол: M / F / пусто.");
            if (p.branch != "paternal" && p.branch != "maternal" && p.branch != "center")
                throw std::runtime_error("Неизвестная ветвь: " + p.branch);
        }
        std::set<std::string> keys;
        for (const auto& r : rs) {
            if (!index.count(r.src) || !index.count(r.tgt)) throw std::runtime_error("Связь с отсутствующим ID.");
            if (r.src == r.tgt) throw std::runtime_error("Связь человека с самим собой.");
            if (r.type != "parent" && r.type != "marriage" && r.type != "sibling")
                throw std::runtime_error("Неизвестный тип связи: " + r.type);
            auto a = r.src, b = r.tgt;
            if (r.type != "parent" && b < a) std::swap(a, b);
            if (!keys.insert(r.type + "\x1f" + a + "\x1f" + b).second)
                throw std::runtime_error("Повторяющаяся связь.");
            if (r.type == "parent" && index.at(r.src)->gen >= index.at(r.tgt)->gen)
                throw std::runtime_error("Родитель должен иметь меньшее поколение: " + r.src + " -> " + r.tgt);
        }
        // Строгое возрастание поколения на parent-рёбрах исключает циклы.
    }


    void load() {
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("Не удалось открыть проект.");
        std::stringstream buffer; buffer << file.rdbuf();
        if (file.bad()) throw std::runtime_error("Ошибка чтения проекта.");
        JParser parser(buffer.str());
        JVal root = parser.parse();
        if (root.t != JVal::OBJ || !root.has("people") || !root.has("relations") ||
            root.at("people").t != JVal::ARR || root.at("relations").t != JVal::ARR)
            throw std::runtime_error("Нужен объект с массивами people и relations.");
        std::vector<Person> ps; std::vector<Relation> rs;
        for (const auto& v : root.at("people").a) {
            if (v.t != JVal::OBJ || !v.has("generation") || v.at("generation").t != JVal::NUM)
                throw std::runtime_error("Неверная запись человека.");
            const double n = v.at("generation").n;
            if (n < 0 || n > 1000 || n != std::floor(n)) throw std::runtime_error("Неверное поколение.");
            Person p;
            p.id = v.str("id"); p.name = v.str("name"); p.sex = v.str("sex");
            p.branch = v.str("branch"); p.maiden = v.str("maiden"); p.dates = v.str("dates");
            p.gen = static_cast<int>(n); ps.push_back(p);
        }
        for (const auto& v : root.at("relations").a) {
            if (v.t != JVal::OBJ) throw std::runtime_error("Неверная запись связи.");
            rs.push_back({v.str("type"), v.str("source"), v.str("target")});
        }
        validate_data(ps, rs);
        people.swap(ps); relations.swap(rs); last_list.clear();
    }

    void save() {
        validate_data(people, relations);
        fs::path temporary = path; temporary += ".tmp";
        std::ofstream f(temporary, std::ios::binary | std::ios::trunc);
        if (!f) throw std::runtime_error("Не удалось создать временный файл проекта.");
        f << "{\n  \"people\": [\n";
        for (size_t i = 0; i < people.size(); ++i) {
            const auto& p = people[i];
            f << "    {\"id\": \"" << jesc(p.id) << "\", \"name\": \"" << jesc(p.name)
              << "\", \"sex\": \"" << jesc(p.sex) << "\", \"generation\": " << p.gen
              << ", \"branch\": \"" << jesc(p.branch) << "\"";
            if (!p.maiden.empty()) f << ", \"maiden\": \"" << jesc(p.maiden) << "\"";
            if (!p.dates.empty())  f << ", \"dates\": \""  << jesc(p.dates)  << "\"";
            f << "}" << (i + 1 < people.size() ? "," : "") << "\n";
        }
        f << "  ],\n  \"relations\": [\n";
        for (size_t i = 0; i < relations.size(); ++i) {
            const auto& r = relations[i];
            f << "    {\"type\": \"" << jesc(r.type) << "\", \"source\": \"" << jesc(r.src)
              << "\", \"target\": \"" << jesc(r.tgt) << "\"}"
              << (i + 1 < relations.size() ? "," : "") << "\n";
        }
        f << "  ]\n}\n";

        f.flush();
        if (!f) throw std::runtime_error("Ошибка записи: исходный файл не изменён.");
        f.close();
        if (!f) throw std::runtime_error("Ошибка закрытия временного файла.");
        if (fs::exists(path)) {
            fs::path backup = path; backup += ".bak";
            fs::copy_file(path, backup, fs::copy_options::overwrite_existing);
        }
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(), path.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Не удалось заменить файл проекта. Старый файл сохранён.");
#else
        fs::rename(temporary, path);
#endif
    }

    void seed_default() {
        auto add = [&](const std::string& id, const std::string& n, const std::string& s,
                       int g, const std::string& br, const std::string& d = "") {
            Person p; p.id = id; p.name = n; p.sex = s; p.gen = g; p.branch = br; p.dates = d;
            people.push_back(p);
        };
        auto rel = [&](const std::string& t, const std::string& a, const std::string& b) {
            relations.push_back({t, a, b});
        };
        add("nabiulla","Набиулла","M",0,"paternal");
        add("tabris","Табрис","M",0,"paternal");
        add("zinatulla","Зинатулла Набиуллин","M",1,"paternal");
        add("latifa","Латифа Хазиева","F",1,"paternal");
        add("rabiga","Рабига Батталова","F",2,"paternal");
        add("tukhvatulla","Тухватулла Зинатуллин","M",2,"paternal");
        add("gilmuilla","Гильмулла Зинатуллин","M",2,"paternal");
        add("sajida","Саҗида Харисова","F",2,"paternal");
        add("mazhar","Мазһар Зинатуллин","M",2,"paternal");
        add("mansur","Мансур Зинатуллин","M",2,"paternal","1897–1986");
        add("mingnegayan","Миңнегаян","F",2,"paternal");
        add("rifgat","Рифгат Зинатуллин","M",3,"paternal");
        add("nailya_n","Наиля Низамутдинова","F",3,"paternal");
        add("ashat","Асхат Зинатуллин","M",3,"paternal");
        add("shamsemukhamat","Шамсемухамат Баишев","M",1,"maternal");
        add("rakiya","Ракия Мустафина","F",1,"maternal");
        add("shayakhmat","Шаяхмат Баишев","M",1,"maternal");
        add("fariza","Фариза Баишева","F",1,"maternal");
        add("rauza","Рауза Баишева","F",2,"maternal","1917–2007");
        add("gabdelkhay","Габделхай Баишев","M",2,"maternal");
        add("mazhit","Мажит Баишев","M",2,"maternal");
        add("asma","Асма Хаҗиева","F",2,"maternal");
        add("ibragim","Ибрагим Баишев","M",2,"maternal");
        add("ismagil","Исмагил Баишев","M",2,"maternal");
        add("gahadiya","Гаһадия Баишева","F",2,"maternal");
        add("ilgiz","Ильгиз","M",3,"maternal");
        add("kamil","Камиль","M",2,"maternal");
        add("ramil_f","Рамиль Фаризов","M",2,"maternal");
        add("zulfira","Зульфира","F",2,"maternal");
        add("rufina_n","Руфина Набиева","F",2,"maternal");
        add("khanif","Ханиф Набиев","M",2,"maternal");
        add("ildar_n","Ильдар Набиев","M",3,"maternal");
        add("irina_n","Ирина Набиева","F",3,"maternal");
        add("zaynetdin","Зайнетдин Фаттахов","M",2,"maternal");
        add("fanuza","Фануза Фаттахова","F",3,"maternal","1936–1957");
        add("kashafetdin","Кашафетдин Фаттахов","M",3,"maternal","1938–2012");
        add("nagima","Нагима Мухарлямова","F",3,"maternal");
        add("raya","Рая","F",3,"maternal");
        add("ratmir","Ратмир Мухарлямов","M",4,"maternal");
        add("olga","Ольга","F",4,"maternal");
        add("konstantin","Константин Фаттахов","M",4,"maternal");
        add("grigory","Григорий","M",5,"maternal");
        add("khaybrakhman","Хайбрахман Хаҗиев","M",2,"maternal");
        add("zubarzhat","Зубаржат Мухарямова","F",3,"maternal");
        add("rashit","Рашит Хаҗиев","M",3,"maternal");
        add("ramil_kh","Рамиль Хаҗиев","M",3,"maternal");
        add("ramazan","Рамазан Мухарямов","M",3,"maternal");
        add("rufina_yu","Руфина Юмагужина","F",4,"maternal");
        add("zulfiya_f","Зульфия Феденькова","F",4,"maternal");
        add("liliya","Лилия Мухарямова","F",4,"maternal");
        add("vil","Виль Юмагужин","M",4,"maternal");
        add("albert","Альберт Юмагужин","M",5,"maternal");
        add("airat","Айрат Юмагужин","M",5,"maternal");
        add("muzh_fed","Муж Феденьков","M",4,"maternal");
        add("ruslan_f","Руслан Феденьков","M",5,"maternal");
        add("ramilya","Рамиля","F",3,"maternal");
        add("banat","Банат","F",3,"maternal");
        add("alishera","Алишер Хаҗиев","M",4,"maternal");
        add("rozaliya","Розалия","F",3,"maternal");
        add("timur","Тимур Хаҗиев","M",4,"maternal");
        add("gadelya","Гаделя Хаҗиева","F",4,"maternal");
        add("arslan","Арслан","M",5,"maternal");
        add("maiya","Майя","F",5,"maternal");
        add("venera","Венера Шабанова","F",3,"center");
        people.back().maiden = "Зинатуллина";
        add("rim","Рим Шабанов","M",3,"center","1939–2004");
        add("dinar","Динар Шабанов","M",4,"center","р. 1968");
        add("ruslan","Руслан Шабанов","M",4,"center","р. 1983");
        add("alina","Алина Руслякова","F",5,"center","р. 1992");
        add("marat","Марат Шабанов","M",5,"center","р. 2005");
        rel("parent","nabiulla","zinatulla");
        rel("parent","tabris","latifa");
        rel("marriage","zinatulla","latifa");
        for (auto c : {"rabiga","tukhvatulla","gilmuilla","sajida","mazhar","mansur"}) {
            rel("parent","zinatulla",c); rel("parent","latifa",c);
        }
        rel("marriage","mansur","mingnegayan");
        for (auto c : {"rifgat","nailya_n","ashat"}) {
            rel("parent","mansur",c); rel("parent","mingnegayan",c);
        }
        rel("marriage","shamsemukhamat","rakiya");
        for (auto c : {"rauza","gabdelkhay","mazhit","asma"}) {
            rel("parent","shamsemukhamat",c); rel("parent","rakiya",c);
        }
        for (auto c : {"ibragim","ismagil","gahadiya"}) rel("parent","shayakhmat",c);
        rel("parent","gahadiya","ilgiz");
        for (auto c : {"kamil","ramil_f","zulfira","rufina_n"}) rel("parent","fariza",c);
        rel("marriage","rufina_n","khanif");
        rel("parent","rufina_n","ildar_n"); rel("parent","khanif","ildar_n");
        rel("parent","rufina_n","irina_n"); rel("parent","khanif","irina_n");
        rel("marriage","rauza","zaynetdin");
        rel("parent","rauza","fanuza"); rel("parent","zaynetdin","fanuza");
        rel("parent","rauza","kashafetdin"); rel("parent","zaynetdin","kashafetdin");
        rel("marriage","kashafetdin","nagima");
        rel("marriage","kashafetdin","raya");
        rel("parent","kashafetdin","ratmir"); rel("parent","nagima","ratmir");
        rel("parent","kashafetdin","olga");   rel("parent","raya","olga");
        rel("parent","kashafetdin","konstantin"); rel("parent","raya","konstantin");
        rel("parent","olga","grigory");
        rel("marriage","asma","khaybrakhman");
        for (auto c : {"zubarzhat","rashit","ramil_kh"}) {
            rel("parent","asma",c); rel("parent","khaybrakhman",c);
        }
        rel("marriage","zubarzhat","ramazan");
        for (auto c : {"rufina_yu","zulfiya_f","liliya"}) {
            rel("parent","zubarzhat",c); rel("parent","ramazan",c);
        }
        rel("marriage","rufina_yu","vil");
        rel("parent","rufina_yu","albert"); rel("parent","vil","albert");
        rel("parent","rufina_yu","airat");  rel("parent","vil","airat");
        rel("marriage","zulfiya_f","muzh_fed");
        rel("parent","zulfiya_f","ruslan_f"); rel("parent","muzh_fed","ruslan_f");
        rel("marriage","rashit","ramilya");
        rel("marriage","rashit","banat");
        rel("parent","rashit","alishera"); rel("parent","ramilya","alishera");
        rel("marriage","ramil_kh","rozaliya");
        rel("parent","ramil_kh","timur"); rel("parent","rozaliya","timur");
        rel("parent","ramil_kh","gadelya"); rel("parent","rozaliya","gadelya");
        rel("parent","gadelya","arslan"); rel("parent","gadelya","maiya");
        rel("marriage","mansur","rauza");
        rel("parent","mansur","venera"); rel("parent","rauza","venera");
        rel("marriage","venera","rim");
        rel("parent","venera","dinar"); rel("parent","rim","dinar");
        rel("parent","venera","ruslan"); rel("parent","rim","ruslan");
        rel("parent","dinar","alina");
        rel("parent","ruslan","marat");
    }

    Person* find(const std::string& id) {
        for (auto& p : people) if (p.id == id) return &p;
        return nullptr;
    }

    std::string next_id() {
        int n = 1;
        while (find("p" + std::to_string(n))) ++n;
        return "p" + std::to_string(n);
    }

    std::vector<Person*> search(const std::string& q) {
        std::vector<Person*> r;
        std::string lq = q;
        std::transform(lq.begin(), lq.end(), lq.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        for (auto& p : people) {
            std::string ln = p.name;
            std::transform(ln.begin(), ln.end(), ln.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (ln.find(lq) != std::string::npos) r.push_back(&p);
        }
        return r;
    }

    std::string resolve(const std::string& arg) {
        if (arg.empty()) return "";
        if (digits(arg)) {
            int idx = std::stoi(arg) - 1;
            if (idx >= 0 && idx < (int)last_list.size()) return last_list[idx];
            if (find("p" + arg)) return "p" + arg;
        }
        if (find(arg)) return arg;
        auto m = search(arg);
        if (m.size() == 1) return m[0]->id;
        if (m.size() > 1) {
            std::cout << YLW << "Несколько совпадений:\n" << RST;
            for (auto* p : m) std::cout << "  " << p->id << "  " << p->name << "\n";
            return "";
        }
        std::cout << RED << "Не найден: " << arg << RST << "\n";
        return "";
    }


    void cmd_help() {
        std::vector<std::string> rows = {styled("КОМАНДЫ", std::string(BLD) + WHT), "\x01"};
        const char* commands[][2] = {
            {"list [ветвь] [пок.]", "список персон"},
            {"tree [ветвь]", "дерево; супруги — ссылки"},
            {"show <id|номер>", "карточка с родственниками"},
            {"add", "добавить человека"}, {"edit <id|номер>", "изменить"},
            {"del <id|номер>", "удалить"}, {"link", "добавить связь"},
            {"unlink <id1> <id2> [тип]", "по умолчанию parent"},
            {"search <текст>", "поиск по имени"},
            {"path <id1> <id2>", "кратчайший путь по связям"},
            {"stats", "статистика"}, {"export <файл.json>", "копия проекта"},
            {"import <файл.json>", "замена текущих данных"},
            {"help / ?", "справка"}, {"q / quit / exit", "выход"}
        };
        for (const auto& c : commands)
            rows.push_back(styled(pad_to(c[0], 29), YLW) + c[1]);
        std::cout << '\n'; panel(rows);
        std::cout << "Ветви: paternal / maternal / center. Пути с пробелами — в кавычках.\n\n";
    }


    void cmd_list(const std::vector<std::string>& args) {
        std::string branch; int generation = -1;
        for (const auto& arg : args) {
            if (arg == "paternal" || arg == "maternal" || arg == "center") branch = arg;
            else generation = generation_number(arg);
        }
        std::vector<Person*> selected;
        last_list.clear();
        for (auto& p : people)
            if ((branch.empty() || p.branch == branch) &&
                (generation < 0 || p.gen == generation)) selected.push_back(&p);
        std::sort(selected.begin(), selected.end(), [](const Person* a, const Person* b) {
            return a->gen != b->gen ? a->gen < b->gen : a->name < b->name;
        });
        std::vector<std::string> rows = {
            styled("СПИСОК (" + std::to_string(selected.size()) + ")" +
                (branch.empty() ? "" : " · " + branch), std::string(BLD) + WHT)
        };
        int previous = -1;
        for (size_t i = 0; i < selected.size(); ++i) {
            const auto& p = *selected[i]; last_list.push_back(p.id);
            if (previous != p.gen) {
                previous = p.gen; rows.push_back("\x01");
                rows.push_back(styled("── Поколение " + std::to_string(p.gen) + " ──", gen_color(p.gen)));
            }
            std::string row = pad_to(std::to_string(i + 1), 3) + " " + sex_icon(p.sex) + "  "
                + styled(p.name, gen_color(p.gen));
            if (p.id == "venera") row += styled(" ★", YLW);
            if (!p.maiden.empty()) row += styled(" (урожд. " + p.maiden + ")", DIM);
            if (!p.dates.empty()) row += styled(" [" + p.dates + "]", DIM);
            rows.push_back(row);
        }
        std::cout << '\n'; panel(rows); std::cout << '\n';
    }


    void cmd_tree(const std::vector<std::string>& args) {
        std::string branch;
        if (!args.empty()) {
            branch = args[0];
            if (branch != "paternal" && branch != "maternal" && branch != "center")
                throw std::runtime_error("Ветвь: paternal / maternal / center.");
        }
        std::map<std::string, Person*> byid;
        std::set<std::string> included;
        for (auto& p : people) {
            byid[p.id] = &p;
            if (branch.empty() || p.branch == branch) included.insert(p.id);
        }
        std::map<std::string, std::vector<std::string>> parents, spouses, kids;
        for (const auto& r : relations) {
            if (!included.count(r.src) || !included.count(r.tgt)) continue;
            if (r.type == "parent") parents[r.tgt].push_back(r.src);
            if (r.type == "marriage") {
                spouses[r.src].push_back(r.tgt); spouses[r.tgt].push_back(r.src);
            }
        }
        auto less = [&](const std::string& a, const std::string& b) {
            return byid.at(a)->gen != byid.at(b)->gen
                ? byid.at(a)->gen < byid.at(b)->gen : byid.at(a)->name < byid.at(b)->name;
        };
        // Единственный основной родитель нужен только для текстовой проекции.
        // Вторые родители перечисляются явно и остаются в исходных связях.
        std::map<std::string, std::string> primary;
        for (auto& entry : parents) {
            auto &v = entry.second;
            std::sort(v.begin(), v.end(), less);
            auto it = std::find_if(v.begin(), v.end(), [&](const std::string& id) { return byid.at(id)->sex == "M"; });
            primary[entry.first] = it == v.end() ? v.front() : *it;
            kids[primary[entry.first]].push_back(entry.first);
        }
        std::vector<std::string> roots;
        for (const auto& id : included) if (!primary.count(id)) roots.push_back(id);
        std::sort(roots.begin(), roots.end(), less);
        for (auto& entry : kids) std::sort(entry.second.begin(), entry.second.end(), less);
        std::set<std::string> visited;
        size_t count = 0;
        std::cout << '\n';
        panel({styled("ДЕРЕВО" + (branch.empty() ? std::string() : " · " + branch), std::string(BLD) + WHT)});
        std::function<void(const std::string&, const std::string&, bool, bool)> print;
        print = [&](const std::string& id, const std::string& prefix, bool last, bool root) {
            if (!visited.insert(id).second) return;
            ++count;
            const auto& p = *byid.at(id);
            std::string connector = root ? "◆ " : last ? "╰─ " : "├─ ";
            std::cout << prefix << styled(connector, CYN) << sex_icon(p.sex) << " "
                << styled(p.name, gen_color(p.gen) + BLD)
                << (p.id == "venera" ? styled(" ★", YLW) : "");
            if (!p.dates.empty()) std::cout << styled(" [" + p.dates + "]", DIM);
            if (!spouses[id].empty()) {
                std::cout << styled("  ♥ ", MAG);
                bool first = true;
                for (const auto& spouse : spouses[id]) {
                    if (!first) {
                        std::cout << ", ";
                    }
                    first = false;
                    std::cout << byid.at(spouse)->name;
                }
            }
            for (const auto& other : parents[id])
                if (primary.count(id) && primary[id] != other)
                    std::cout << styled("  [также родитель: " + byid.at(other)->name + "]", DIM);
            std::cout << '\n';
            auto& children = kids[id];
            const std::string next = prefix + (root ? "   " : last ? "   " : styled("│", CYN) + "  ");
            for (size_t i = 0; i < children.size(); ++i)
                print(children[i], next, i + 1 == children.size(), false);
        };
        for (const auto& id : roots) { print(id, "", true, true); std::cout << '\n'; }
        for (const auto& id : included)
            if (!visited.count(id)) print(id, "", true, true);
        std::cout << DIM << "Показано персон: " << count << " / " << included.size() << RST << '\n';
        std::cout << "Легенда: " << sex_icon("M") << " муж  " << sex_icon("F")
            << " жен  " << styled("★", YLW) << " центр  " << styled("♥", MAG) << " брак\n"
            << "Супруги рядом с именем — ссылки; каждый человек имеет свою строку.\n"
            << "Фильтр ветви скрывает людей и связи за её пределами.\n\n";
    }


    void cmd_show(const std::string& id) {
        auto* p = find(id);
        if (!p) throw std::runtime_error("Человек не найден.");
        std::vector<std::string> rows = {
            styled((id == "venera" ? "★ " : "") + p->name, std::string(BLD) + WHT),
            styled("ID: " + id, DIM)
        };
        if (!p->maiden.empty()) rows.push_back("урожд. " + p->maiden);
        rows.push_back("\x01");
        rows.push_back(sex_icon(p->sex) + "  Поколение: " + std::to_string(p->gen) +
            "  ·  " + styled(p->branch, branch_color(p->branch)));
        if (!p->dates.empty()) rows.push_back("Даты: " + p->dates);
        std::vector<std::string> par, chi, spo, sib;
        for (const auto& r : relations) {
            if (r.type == "parent") {
                if (r.tgt == id) par.push_back(r.src);
                if (r.src == id) chi.push_back(r.tgt);
            } else {
                auto& target = r.type == "marriage" ? spo : sib;
                if (r.src == id) target.push_back(r.tgt);
                else if (r.tgt == id) target.push_back(r.src);
            }
        }
        auto block = [&](const std::string& title, const std::vector<std::string>& ids) {
            if (ids.empty()) return;
            rows.push_back("\x01");
            rows.push_back(styled(title + " (" + std::to_string(ids.size()) + ")", std::string(BLD) + YLW));
            for (const auto& rid : ids) {
                auto* relative = find(rid); if (!relative) continue;
                std::string text = sex_icon(relative->sex) + " " + relative->name;
                if (!relative->dates.empty()) text += " [" + relative->dates + "]";
                rows.push_back(text + styled(" (" + rid + ")", DIM));
            }
        };
        block("↑ Родители", par); block("♥ Супруг(а)", spo);
        block("↓ Дети", chi); block("≈ Братья / сёстры", sib);
        std::cout << '\n'; panel(rows); std::cout << '\n';
    }

    void cmd_add() {
        std::cout << CYN << "Новый человек (пустой ввод — отмена)\n" << RST;
        auto ask = [&](const std::string& p) {
            std::cout << "  " << p; std::string s; std::getline(std::cin, s); return s;
        };
        std::string name = ask("Имя и фамилия: ");
        if (name.empty()) { std::cout << YLW << "Отменено\n" << RST; return; }
        std::string sex = ask("Пол [M/F]: ");
        std::string gen_s = ask("Поколение [0]: ");
        std::string branch = ask("Ветвь [paternal/maternal/center]: ");
        if (branch.empty()) branch = "center";
        std::string maiden = ask("Урожд. фамилия: ");
        std::string dates = ask("Годы: ");
        Person p;
        p.id = next_id(); p.name = name; p.sex = sex; p.branch = branch;
        p.gen = gen_s.empty() ? 0 : generation_number(gen_s);
        p.maiden = maiden; p.dates = dates;
        auto candidate = people; candidate.push_back(p);
        validate_data(candidate, relations);
        people.swap(candidate);
        save();
        std::cout << GRN << "✓ Добавлено: " << p.name << " (id=" << p.id << ")\n" << RST;
    }

    void cmd_edit(const std::string& id) {
        auto* p = find(id);
        if (!p) { std::cout << RED << "Не найден\n" << RST; return; }
        auto ask = [&](const std::string& pr, const std::string& cur) {
            std::cout << "  " << pr << " [" << cur << "]: ";
            std::string s; std::getline(std::cin, s); return s.empty() ? cur : s;
        };
        std::string name = ask("Имя", p->name);
        std::string sex = ask("Пол", p->sex);
        std::string gen_s = ask("Поколение", std::to_string(p->gen));
        std::string branch = ask("Ветвь", p->branch);
        std::string maiden = ask("Урожд.", p->maiden);
        std::string dates = ask("Годы", p->dates);
        Person updated = *p;
        updated.name = name; updated.sex = sex; updated.branch = branch;
        updated.gen = generation_number(gen_s); updated.maiden = maiden; updated.dates = dates;
        auto candidate = people;
        for (auto& value : candidate) if (value.id == id) value = updated;
        validate_data(candidate, relations);
        people.swap(candidate);
        save();
        std::cout << GRN << "✓ Сохранено\n" << RST;
    }

    void cmd_del(const std::string& id) {
        auto* p = find(id);
        if (!p) { std::cout << RED << "Не найден\n" << RST; return; }
        std::cout << "Удалить " << BLD << p->name << RST << " и все связи? [y/N]: ";
        std::string s; std::getline(std::cin, s);
        if (s != "y" && s != "Y") { std::cout << YLW << "Отменено\n" << RST; return; }
        people.erase(std::remove_if(people.begin(), people.end(),
            [&](const Person& x){ return x.id == id; }), people.end());
        relations.erase(std::remove_if(relations.begin(), relations.end(),
            [&](const Relation& r){ return r.src == id || r.tgt == id; }), relations.end());
        last_list.clear();
        save();
        std::cout << GRN << "✓ Удалено\n" << RST;
    }

    void cmd_link() {
        std::cout << CYN << "Тип: parent, marriage, sibling\n" << RST;
        std::cout << "  Тип [parent]: "; std::string type; std::getline(std::cin, type);
        if (type.empty()) type = "parent";
        std::cout << "  Первый (id|номер): "; std::string a; std::getline(std::cin, a);
        std::cout << "  Второй (id|номер): "; std::string b; std::getline(std::cin, b);
        std::string ia = resolve(a), ib = resolve(b);
        if (ia.empty() || ib.empty()) return;
        if (ia == ib) { std::cout << RED << "Нельзя связать с самим собой\n" << RST; return; }
        for (auto& r : relations) {
            if (r.type != type) continue;
            if (type == "parent") {
                if (r.src == ia && r.tgt == ib) { std::cout << YLW << "Уже есть\n" << RST; return; }
            } else {
                if ((r.src == ia && r.tgt == ib) || (r.src == ib && r.tgt == ia)) {
                    std::cout << YLW << "Уже есть\n" << RST; return;
                }
            }
        }
        if (type == "parent") {
            auto* pa = find(ia); auto* pb = find(ib);
            if (pa && pb && pa->gen >= pb->gen) {
                std::cout << RED << "Поколение родителя должно быть меньше поколения ребёнка\n" << RST;
                return;
            }
        }
        auto candidate = relations; candidate.push_back({type, ia, ib});
        validate_data(people, candidate);
        relations.swap(candidate);
        save();
        std::cout << GRN << "✓ Связь добавлена\n" << RST;
    }

    void cmd_unlink(const std::vector<std::string>& a) {
        if (a.size() < 2) { std::cout << RED << "unlink <id1> <id2> [тип]\n" << RST; return; }
        std::string ia = resolve(a[0]), ib = resolve(a[1]);
        if (ia.empty() || ib.empty()) return;
        std::string type = a.size() > 2 ? a[2] : "parent";
        size_t before = relations.size();
        relations.erase(std::remove_if(relations.begin(), relations.end(),
            [&](const Relation& r){
                if (r.type != type) return false;
                if (type == "parent") return r.src == ia && r.tgt == ib;
                return (r.src == ia && r.tgt == ib) || (r.src == ib && r.tgt == ia);
            }), relations.end());
        if (relations.size() < before) { save(); std::cout << GRN << "✓ Удалено\n" << RST; }
        else std::cout << YLW << "Связь не найдена\n" << RST;
    }

    void cmd_search(const std::string& q) {
        last_list.clear();
        auto res = search(q);
        if (res.empty()) { std::cout << YLW << "Ничего не найдено\n" << RST; return; }
        last_list.clear();
        for (auto* p : res) last_list.push_back(p->id);
        std::cout << "\n" << BLD << CYN << "НАЙДЕНО (" << res.size() << ")" << RST << "\n";
        for (size_t i = 0; i < res.size(); ++i) {
            auto* p = res[i];
            std::cout << "  " << (i + 1) << ". "
                      << (p->sex == "M" ? "♂" : p->sex == "F" ? "♀" : "·") << " " << p->name;
            if (!p->dates.empty()) std::cout << "  [" << p->dates << "]";
            std::cout << "  " << DIM << "(" << p->id << ")" << RST << "\n";
        }
        std::cout << "\n";
    }

    void cmd_path(const std::string& a, const std::string& b) {
        std::string ia = resolve(a), ib = resolve(b);
        if (ia.empty() || ib.empty()) return;
        std::map<std::string, std::set<std::string>> adj;
        for (auto& r : relations) { adj[r.src].insert(r.tgt); adj[r.tgt].insert(r.src); }
        std::map<std::string, std::string> prev;
        std::vector<std::string> q; prev[ia] = ""; q.push_back(ia);
        for (size_t i = 0; i < q.size(); ++i) {
            auto cur = q[i];
            if (cur == ib) break;
            for (auto& nx : adj[cur]) if (!prev.count(nx)) { prev[nx] = cur; q.push_back(nx); }
        }
        if (!prev.count(ib)) { std::cout << YLW << "Путь не найден\n" << RST; return; }
        std::vector<std::string> path;
        std::string cur = ib;
        while (!cur.empty()) { path.push_back(cur); cur = prev[cur]; }
        std::reverse(path.begin(), path.end());
        std::cout << "\n" << BLD << CYN << "ПУТЬ (" << (path.size() - 1) << " шагов)" << RST << "\n";
        for (size_t i = 0; i < path.size(); ++i) {
            auto* p = find(path[i]); if (!p) continue;
            std::cout << "  " << (i ? "↓ " : "")
                      << (p->sex == "M" ? "♂" : p->sex == "F" ? "♀" : "·")
                      << " " << p->name << "\n";
        }
        std::cout << "\n";
    }


    void cmd_stats() {
        int males = 0, females = 0;
        std::set<int> generations;
        std::map<std::string, int> branches, types;
        for (const auto& p : people) {
            males += p.sex == "M"; females += p.sex == "F";
            generations.insert(p.gen); ++branches[p.branch];
        }
        for (const auto& r : relations) ++types[r.type];
        std::vector<std::string> rows = {
            styled("СТАТИСТИКА", std::string(BLD) + WHT), "\x01",
            "Всего персон:       " + std::to_string(people.size()),
            "Мужчин / женщин:    " + std::to_string(males) + " / " + std::to_string(females),
            "Пол не указан:      " + std::to_string(people.size() - males - females),
            "Поколений:          " + std::to_string(generations.size()),
            "\x01", styled("По ветвям:", YLW)
        };
        for (const auto& entry : branches)
            rows.push_back(styled(branch_mark(entry.first) + "  " + entry.first, branch_color(entry.first))
                + "   " + std::to_string(entry.second));
        rows.push_back("\x01"); rows.push_back(styled("По связям:", YLW));
        for (const auto& entry : types) rows.push_back(entry.first + "   " + std::to_string(entry.second));
        std::cout << '\n'; panel(rows); std::cout << '\n';
    }


    void cmd_export(const std::string& filename) {
        const fs::path original = path;
        try { path = fs::u8path(filename); save(); }
        catch (...) { path = original; throw; }
        path = original;
        std::cout << GRN << "✓ Копия сохранена: " << filename << RST << "\n";
    }


    void cmd_import(const std::string& filename) {
        const fs::path source = fs::u8path(filename);
        if (!fs::exists(source)) throw std::runtime_error("Файл не найден: " + filename);
        std::cout << "Заменить данные текущего проекта? [y/N]: ";
        std::string answer; std::getline(std::cin, answer);
        if (answer != "y" && answer != "Y") return;
        const fs::path original = path;
        const auto oldPeople = people; const auto oldRelations = relations;
        const auto oldList = last_list;
        try {
            path = source; load(); path = original; save();
        } catch (...) {
            path = original; people = oldPeople; relations = oldRelations;
            last_list = oldList; throw;
        }
        std::cout << GRN << "✓ Импортировано в текущий проект\n" << RST;
    }

    void run() {
        std::cout << "\n";
        panel({styled("◆ ГЕНЕАЛОГИЧЕСКОЕ ДЕРЕВО", std::string(BLD) + WHT),
               "Шабанова (Зинатуллина) Венера Мансуровна",
               "Отцовская линия · Материнская линия"});
        std::cout << "Данные: " << path << "\nПерсон: " << people.size()
                  << "   Связей: " << relations.size() << "\n";
        cmd_help();
        std::string line;
        while (true) {
            std::cout << GRN << BLD << "ftree> " << RST;
            if (!std::getline(std::cin, line)) break;
            if (line.empty()) continue;
            try {
            auto tokens = split_command(line);
            if (tokens.empty()) continue;
            const std::string cmd = tokens.front();
            std::vector<std::string> args(tokens.begin() + 1, tokens.end());

            if (cmd == "help" || cmd == "?") cmd_help();
            else if (cmd == "list" || cmd == "ls") cmd_list(args);
            else if (cmd == "tree") cmd_tree(args);
            else if (cmd == "show" && !args.empty()) {
                auto id = resolve(args[0]); if (!id.empty()) cmd_show(id);
            }
            else if (cmd == "add") cmd_add();
            else if (cmd == "edit" && !args.empty()) {
                auto id = resolve(args[0]); if (!id.empty()) cmd_edit(id);
            }
            else if (cmd == "del" || cmd == "delete" || cmd == "rm") {
                if (args.empty()) { std::cout << RED << "del <id>\n" << RST; continue; }
                auto id = resolve(args[0]); if (!id.empty()) cmd_del(id);
            }
            else if (cmd == "link") cmd_link();
            else if (cmd == "unlink") cmd_unlink(args);
            else if ((cmd == "search" || cmd == "find") && !args.empty()) {
                std::string q; for (auto& a : args) { if (!q.empty()) q += " "; q += a; }
                cmd_search(q);
            }
            else if (cmd == "path" && args.size() >= 2) cmd_path(args[0], args[1]);
            else if (cmd == "stats") cmd_stats();
            else if (cmd == "export" && !args.empty()) cmd_export(args[0]);
            else if (cmd == "import" && !args.empty()) cmd_import(args[0]);
            else if (cmd == "quit" || cmd == "exit" || cmd == "q") break;
            else std::cout << RED << "Неизвестная команда: " << cmd << ". help — справка.\n" << RST;
            } catch (const std::exception& error) {
                std::cout << RED << "Ошибка: " << error.what() << RST << "\n";
            }
        }
    }
};

int main() {
    setup_console();
    try {
        FamilyTree t;
        t.run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << RED << "Ошибка запуска: " << error.what() << RST << "\n";
        std::cerr << "Исходный JSON не перезаписан. Проверьте файл и резервную копию.\n";
        return 1;
    }
}