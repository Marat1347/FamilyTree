import json
import os
import subprocess

DATA_FILE = r"C:\python\Scripts\F_TREE\data\family_data.json"

with open(DATA_FILE, "r", encoding="utf-8") as f:
    data = json.load(f)

people = data["people"]
relations = data["relations"]

existing_ids = {p["id"] for p in people}

def add_person(id, name, sex, generation, branch, maiden="", dates=""):
    if id in existing_ids:
        return
    people.append({
        "id": id, "name": name, "sex": sex, "generation": generation,
        "branch": branch, "maiden": maiden, "dates": dates
    })
    existing_ids.add(id)

def add_relation(src, tgt, rel_type="parent"):
    for r in relations:
        if r["source"] == src and r["target"] == tgt and r["type"] == rel_type:
            return
    relations.append({"type": rel_type, "source": src, "target": tgt})

# --- ОТЦОВСКАЯ ЛИНИЯ ---
add_person("sultangali", "Султангали Батталов", "M", 4, "paternal")
add_relation("sultangali", "rabiga", "marriage")

add_person("mariam_h", "Мәръям Хайретдинова", "F", 4, "paternal")
add_relation("rabiga", "mariam_h", "parent")
add_person("tauhetdin_h", "Тәухетдин Хайретдинов", "M", 4, "paternal")
add_relation("tauhetdin_h", "mariam_h", "marriage")
for child, name, sex in [("sarim","Сарим","M"), ("airat_h","Айрат","M"), ("amir","Амир","M"), ("asiya","Асия","F"), ("nailya_h","Наиля","F")]:
    add_person(child, name, sex, 5, "paternal")
    add_relation("mariam_h", child, "parent")
    add_relation("tauhetdin_h", child, "parent")

add_person("gabdulla_b", "Габдулла Батталов", "M", 4, "paternal")
add_relation("rabiga", "gabdulla_b", "parent")
add_person("satira", "Сәтирә Батталова", "F", 4, "paternal")
add_relation("satira", "gabdulla_b", "marriage")

add_person("safiya_s", "Сафия Суяргулова", "F", 4, "paternal")
add_relation("rabiga", "safiya_s", "parent")
add_person("islam_s", "Ислам Суяргулов", "M", 4, "paternal")
add_relation("islam_s", "safiya_s", "marriage")
for child, name, sex in [("rauza_s","Рауза","F"), ("rim_s","Рим","M"), ("rurik_s","Рурик","M")]:
    add_person(child, name, sex, 5, "paternal")
    add_relation("safiya_s", child, "parent")
    add_relation("islam_s", child, "parent")

add_person("ahmasultan", "Ахмасултан Батталов", "M", 4, "paternal")
add_relation("rabiga", "ahmasultan", "parent")
add_person("dzhamilya", "Җәмилә Батталова", "F", 4, "paternal")
add_relation("dzhamilya", "ahmasultan", "marriage")
add_person("chulpan", "Чулпан Батталова", "F", 5, "paternal")
add_relation("ahmasultan", "chulpan", "parent")
add_relation("dzhamilya", "chulpan", "parent")
add_person("yuriy_b", "Юрий Батталов", "M", 5, "paternal")
add_relation("ahmasultan", "yuriy_b", "parent")
add_relation("dzhamilya", "yuriy_b", "parent")
add_person("venera_ism", "Венера Исмагиловна", "F", 5, "paternal")
add_relation("venera_ism", "yuriy_b", "marriage")
add_person("marat_b", "Марат Батталов", "M", 6, "paternal")
add_relation("yuriy_b", "marat_b", "parent")
add_relation("venera_ism", "marat_b", "parent")
add_person("guzel_b", "Гюзель Батталова", "F", 6, "paternal")
add_relation("yuriy_b", "guzel_b", "parent")
add_relation("venera_ism", "guzel_b", "parent")
add_person("gabdulhay_b", "Габделхай Батталов", "M", 4, "paternal")
add_relation("rabiga", "gabdulhay_b", "parent")

add_person("sabira_k", "Сабира Каюмова", "F", 4, "paternal")
add_relation("tukhvatulla", "sabira_k", "parent")
add_person("mogin_k", "Мөгин Каюмов", "M", 4, "paternal")
add_relation("mogin_k", "sabira_k", "marriage")

add_person("taufik_z", "Тауфик Зинатуллин", "M", 4, "paternal")
add_relation("tukhvatulla", "taufik_z", "parent")
add_person("kameriya", "Кәмәрия Зинатуллина", "F", 4, "paternal")
add_relation("kameriya", "taufik_z", "marriage")
for child, name, sex in [("rauil","Рауиль","M"), ("fanuza_z","Фәнүзә","F"), ("rauza_z","Рауза","F"), ("rail","Раиль","M"), ("ramil_z","Рамиль","M"), ("rafik","Рафик","M"), ("faik_z","Фаик","M")]:
    add_person(child, name, sex, 5, "paternal")
    add_relation("taufik_z", child, "parent")
    add_relation("kameriya", child, "parent")

add_person("najip_z", "Наҗип Зинатуллин", "M", 4, "paternal")
add_relation("tukhvatulla", "najip_z", "parent")
add_person("dzhavgar", "Җәүһәр Зинатуллина", "F", 4, "paternal")
add_relation("dzhavgar", "najip_z", "marriage")

add_person("faik_z_child", "Фаик Зинатуллин (умер в детстве)", "M", 4, "paternal")
add_relation("tukhvatulla", "faik_z_child", "parent")

add_person("gamilya_g", "Гәмилә Гайсина", "F", 4, "paternal")
add_relation("gilmuilla", "gamilya_g", "parent")
add_person("gabida", "Гәбидә", "F", 5, "paternal")
add_relation("gamilya_g", "gabida", "parent")

add_person("salima_sh", "Сәлимә Шамсутдинова", "F", 4, "paternal")
add_relation("gilmuilla", "salima_sh", "parent")
add_person("haliulla", "Халиулла", "M", 5, "paternal")
add_relation("salima_sh", "haliulla", "parent")
add_person("saniya", "Сания", "F", 5, "paternal")
add_relation("salima_sh", "saniya", "parent")
add_person("rauza_sal", "Рауза", "F", 5, "paternal")
add_relation("salima_sh", "rauza_sal", "parent")
add_person("raifa", "Райфа", "F", 5, "paternal")
add_relation("salima_sh", "raifa", "parent")
add_person("liliya_s", "Лилия", "F", 6, "paternal")
add_relation("saniya", "liliya_s", "parent")
add_person("diana", "Диана", "F", 6, "paternal")
add_relation("rauza_sal", "diana", "parent")

add_person("gabdulla_g", "Габдулла (умер в детстве)", "M", 4, "paternal")
add_relation("gilmuilla", "gabdulla_g", "parent")

add_person("varashan_h", "Вәрашан Харисов", "M", 4, "paternal")
add_relation("sajida", "varashan_h", "parent")
add_person("surur", "Сурур Харисова", "F", 4, "paternal")
add_relation("surur", "varashan_h", "marriage")
add_person("venera_h", "Венера Харисова", "F", 5, "paternal")
add_relation("varashan_h", "venera_h", "parent")
add_relation("surur", "venera_h", "parent")
add_person("alfiya_h", "Альфия Харисова", "F", 5, "paternal")
add_relation("varashan_h", "alfiya_h", "parent")
add_relation("surur", "alfiya_h", "parent")

add_person("halida_s", "Халида Сафиуллина", "F", 4, "paternal")
add_relation("sajida", "halida_s", "parent")
add_person("saib_s", "Саиб Сафиуллин", "M", 4, "paternal")
add_relation("saib_s", "halida_s", "marriage")
add_person("ramziya", "Рамзия", "F", 5, "paternal")
add_relation("halida_s", "ramziya", "parent")
add_relation("saib_s", "ramziya", "parent")
add_person("foat", "Фоат", "M", 5, "paternal")
add_relation("halida_s", "foat", "parent")
add_relation("saib_s", "foat", "parent")

add_person("hallat_h", "Халләт Харисов", "M", 4, "paternal")
add_relation("sajida", "hallat_h", "parent")

add_person("hualit_h", "Хуалит Харисов", "M", 4, "paternal")
add_relation("sajida", "hualit_h", "parent")
add_person("nazira", "Назира Харисова", "F", 4, "paternal")
add_relation("nazira", "hualit_h", "marriage")
add_person("son_hualit", "Сын Хуалита", "M", 5, "paternal")
add_relation("hualit_h", "son_hualit", "parent")
add_relation("nazira", "son_hualit", "parent")
add_person("doch_hualit", "Дочь Хуалита", "F", 5, "paternal")
add_relation("hualit_h", "doch_hualit", "parent")
add_relation("nazira", "doch_hualit", "parent")

add_person("sakina", "Сакина", "F", 4, "paternal")
add_relation("sajida", "sakina", "parent")
add_person("rashida", "Рашида", "F", 4, "paternal")
add_relation("sajida", "rashida", "parent")

add_person("nakit_h", "Накит Харисов", "M", 4, "paternal")
add_relation("sajida", "nakit_h", "parent")
add_person("shahida", "Шаһида Харисова", "F", 4, "paternal")
add_relation("shahida", "nakit_h", "marriage")
add_person("alfiya_n", "Альфия Харисова", "F", 5, "paternal")
add_relation("nakit_h", "alfiya_n", "parent")
add_relation("shahida", "alfiya_n", "parent")
add_person("nail_h", "Наиль Харисов", "M", 5, "paternal")
add_relation("nakit_h", "nail_h", "parent")
add_relation("shahida", "nail_h", "parent")

add_person("dzhavat", "Җәүәт Зинатуллин", "M", 4, "paternal")
add_relation("mazhar", "dzhavat", "parent")
add_person("nadjat", "Наҗәт Зинатуллин", "M", 4, "paternal")
add_relation("mazhar", "nadjat", "parent")
add_person("lajla", "Ләйлә Зинатуллина", "F", 4, "paternal")
add_relation("mazhar", "lajla", "parent")

add_person("galimyan", "Галимьян Низамутдинов", "M", 4, "paternal")
add_relation("galimyan", "nailya_n", "marriage")
add_person("rafail", "Рафаил Низамутдинов", "M", 5, "paternal")
add_relation("nailya_n", "rafail", "parent")
add_relation("galimyan", "rafail", "parent")
add_person("ramil_niz", "Рамиль Низамутдинов", "M", 5, "paternal")
add_relation("nailya_n", "ramil_niz", "parent")
add_relation("galimyan", "ramil_niz", "parent")
add_person("lyudmila", "Людмила Низамутдинова", "F", 5, "paternal")
add_relation("lyudmila", "ramil_niz", "marriage")
add_person("nailya_niz_jr", "Наиля Низамутдинова (младшая)", "F", 6, "paternal")
add_relation("ramil_niz", "nailya_niz_jr", "parent")
add_relation("lyudmila", "nailya_niz_jr", "parent")

add_person("tamara", "Тамара Зинатуллина", "F", 4, "paternal")
add_relation("tamara", "ashat", "marriage")
add_person("oleg", "Олег Зинатуллин", "M", 5, "paternal")
add_relation("ashat", "oleg", "parent")
add_relation("tamara", "oleg", "parent")

add_person("guzel_sh", "Гузель Шабанова", "F", 5, "center")
add_relation("guzel_sh", "dinar", "marriage")
add_person("vladimir", "Владимир Русляков", "M", 6, "center")
add_relation("vladimir", "alina", "marriage")
add_person("natalya", "Наталья Леонидовна", "F", 5, "center")
add_relation("natalya", "ruslan", "marriage")
add_person("regina", "Регина Минебаева", "F", 5, "center")
add_relation("regina", "ruslan", "marriage")

# --- МАТЕРИНСКАЯ ЛИНИЯ ---
add_person("ibragim", "Ибрагим Баишев", "M", 2, "maternal")
add_relation("shayakhmat", "ibragim", "parent")
add_person("ismagil", "Исмагил Баишев", "M", 2, "maternal")
add_relation("shayakhmat", "ismagil", "parent")
add_person("gahadiya", "Гаһадия Баишева", "F", 2, "maternal")
add_relation("shayakhmat", "gahadiya", "parent")
add_person("ilgiz", "Ильгиз", "M", 3, "maternal")
add_relation("gahadiya", "ilgiz", "parent")

add_person("kamil", "Камиль", "M", 2, "maternal")
add_relation("fariza", "kamil", "parent")
add_person("ramil_f", "Рамиль Фаризов", "M", 2, "maternal")
add_relation("fariza", "ramil_f", "parent")
add_person("zulfira", "Зульфира", "F", 2, "maternal")
add_relation("fariza", "zulfira", "parent")
add_person("rufina_n", "Руфина Набиева", "F", 2, "maternal")
add_relation("fariza", "rufina_n", "parent")
add_person("khanif", "Ханиф Набиев", "M", 2, "maternal")
add_relation("khanif", "rufina_n", "marriage")
add_person("ildar_n", "Ильдар Набиев", "M", 3, "maternal")
add_relation("rufina_n", "ildar_n", "parent")
add_relation("khanif", "ildar_n", "parent")
add_person("irina_n", "Ирина Набиева", "F", 3, "maternal")
add_relation("rufina_n", "irina_n", "parent")
add_relation("khanif", "irina_n", "parent")

add_person("fanuza", "Фануза Фаттахова", "F", 3, "maternal")
add_relation("rauza", "fanuza", "parent")
add_person("kashafetdin", "Кашафетдин Фаттахов", "M", 3, "maternal")
add_relation("rauza", "kashafetdin", "parent")
add_person("nagima", "Нагима Мухарлямова", "F", 3, "maternal")
add_relation("nagima", "kashafetdin", "marriage")
add_person("raya", "Рая", "F", 3, "maternal")
add_relation("raya", "kashafetdin", "marriage")
add_person("ratmir", "Ратмир Мухарлямов", "M", 4, "maternal")
add_relation("kashafetdin", "ratmir", "parent")
add_relation("nagima", "ratmir", "parent")
add_person("olga", "Ольга", "F", 4, "maternal")
add_relation("kashafetdin", "olga", "parent")
add_relation("raya", "olga", "parent")
add_person("grigory", "Григорий", "M", 5, "maternal")
add_relation("olga", "grigory", "parent")
add_person("konstantin", "Константин Фаттахов", "M", 4, "maternal")
add_relation("kashafetdin", "konstantin", "parent")
add_relation("raya", "konstantin", "parent")
add_person("doch_konst", "Дочь Константина", "F", 5, "maternal")
add_relation("konstantin", "doch_konst", "parent")

add_person("zubarzhat", "Зубаржат Мухарямова", "F", 3, "maternal")
add_relation("asma", "zubarzhat", "parent")
add_person("ramazan", "Рамазан Мухарямов", "M", 3, "maternal")
add_relation("ramazan", "zubarzhat", "marriage")
add_person("rufina_yu", "Руфина Юмагужина", "F", 4, "maternal")
add_relation("zubarzhat", "rufina_yu", "parent")
add_relation("ramazan", "rufina_yu", "parent")
add_person("zulfiya_f", "Зульфия Феденькова", "F", 4, "maternal")
add_relation("zubarzhat", "zulfiya_f", "parent")
add_relation("ramazan", "zulfiya_f", "parent")
add_person("liliya", "Лилия Мухарямова", "F", 4, "maternal")
add_relation("zubarzhat", "liliya", "parent")
add_relation("ramazan", "liliya", "parent")
add_person("vil", "Виль Юмагужин", "M", 4, "maternal")
add_relation("vil", "rufina_yu", "marriage")
add_person("albert", "Альберт Юмагужин", "M", 5, "maternal")
add_relation("rufina_yu", "albert", "parent")
add_relation("vil", "albert", "parent")
add_person("airat", "Айрат Юмагужин", "M", 5, "maternal")
add_relation("rufina_yu", "airat", "parent")
add_relation("vil", "airat", "parent")
add_person("muzh_fed", "Муж Феденьков", "M", 4, "maternal")
add_relation("muzh_fed", "zulfiya_f", "marriage")
add_person("ruslan_f", "Руслан Феденьков", "M", 5, "maternal")
add_relation("zulfiya_f", "ruslan_f", "parent")
add_relation("muzh_fed", "ruslan_f", "parent")

add_person("rashit", "Рашит Хаҗиев", "M", 3, "maternal")
add_relation("asma", "rashit", "parent")
add_person("ramilya", "Рамиля", "F", 3, "maternal")
add_relation("ramilya", "rashit", "marriage")
add_person("banat", "Банат", "F", 3, "maternal")
add_relation("banat", "rashit", "marriage")
add_person("alishera", "Алишер Хаҗиев", "M", 4, "maternal")
add_relation("rashit", "alishera", "parent")
add_relation("ramilya", "alishera", "parent")

add_person("ramil_kh", "Рамиль Хаҗиев", "M", 3, "maternal")
add_relation("asma", "ramil_kh", "parent")
add_person("rozaliya", "Розалия", "F", 3, "maternal")
add_relation("rozaliya", "ramil_kh", "marriage")
add_person("timur", "Тимур Хаҗиев", "M", 4, "maternal")
add_relation("ramil_kh", "timur", "parent")
add_relation("rozaliya", "timur", "parent")
add_person("gadelya", "Гаделя Хаҗиева", "F", 4, "maternal")
add_relation("ramil_kh", "gadelya", "parent")
add_relation("rozaliya", "gadelya", "parent")
add_person("arslan", "Арслан", "M", 5, "maternal")
add_relation("gadelya", "arslan", "parent")
add_person("maiya", "Майя", "F", 5, "maternal")
add_relation("gadelya", "maiya", "parent")

# Сохранение
data["people"] = people
data["relations"] = relations

with open(DATA_FILE, "w", encoding="utf-8") as f:
    json.dump(data, f, ensure_ascii=False, indent=2)

print(f"✅ Добавлено/обновлено. Всего персон: {len(people)}")

# Запуск make_mobile.py
print("🔄 Обновляю мобильную версию...")
subprocess.run(["C:\\python\\python.exe", "make_mobile.py"], check=True)
print("✅ Мобильная версия обновлена.")