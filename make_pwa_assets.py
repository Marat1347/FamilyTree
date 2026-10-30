import json
import os
from PIL import Image

# 1. Конвертируем иконку из ICO в PNG
ico_path = r"C:\python\Scripts\F_TREE\gui_native\app_icon.ico"
png_path = r"C:\python\Scripts\F_TREE\mobile\icon.png"
if os.path.exists(ico_path):
    img = Image.open(ico_path)
    img.save(png_path, "PNG")
    print("✅ Иконка icon.png создана")
else:
    print("⚠️ Файл app_icon.ico не найден, иконка будет стандартной")

# 2. Создаем manifest.json
manifest = {
    "name": "Семейное дерево",
    "short_name": "Дерево",
    "start_url": "./family_tree.html",
    "display": "standalone",
    "background_color": "#0D1420",
    "theme_color": "#0D1420",
    "icons": [
        {
            "src": "icon.png",
            "sizes": "256x256",
            "type": "image/png"
        }
    ]
}
manifest_path = r"C:\python\Scripts\F_TREE\mobile\manifest.json"
with open(manifest_path, "w", encoding="utf-8") as f:
    json.dump(manifest, f, ensure_ascii=False, indent=2)
print("✅ manifest.json создан")