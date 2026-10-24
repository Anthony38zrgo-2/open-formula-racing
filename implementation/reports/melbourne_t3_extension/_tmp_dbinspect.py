import json
import sqlite3

db = r"D:\open-formula\distribution\user-settings\open_formula.db"
con = sqlite3.connect(f"file:{db}?mode=ro", uri=True)
cur = con.cursor()
cols = [c[1] for c in cur.execute("pragma table_info(car_setup)")]
for row in cur.execute("select * from car_setup"):
    rec = dict(zip(cols, row))
    vj = rec.pop("values_json")
    print({k: v for k, v in rec.items()})
    data = json.loads(vj)
    print("keys:", list(data)[:80])
    print(json.dumps(data, indent=1, ensure_ascii=False)[:3000])
