"""
Configura el entorno aislado de Rally (user-settings-rally) y el racemanager rally-russia-stage1.xml
"""

import shutil
from pathlib import Path

ROOT = Path(r"D:\open-formula")
DIST = ROOT / "distribution"
SHARE = DIST / "share/games/speed-dreams-2"
INITIAL_SETTINGS = DIST / "initial-settings"
RALLY_SETTINGS = DIST / "user-settings-rally"

def setup_raceman():
    raceman_content = """<?xml version="1.0" encoding="utf-8"?>
<params name="Rally Russia Stage 1" version="1.65">
  <section name="Header">
    <attstr name="name" val="Practice" />
    <attstr name="type" val="Practice" />
    <attstr name="description" val="Rally Russia Stage 1 - Ford Escort RS2000" />
    <attnum name="priority" val="100" />
    <attstr name="menu image" val="data/img/splash-practice.jpg" />
    <attstr name="run image" val="data/img/splash-run-practice.jpg" />
  </section>
  <section name="Tracks">
    <attnum name="maximum number" val="1" />
    <section name="1">
      <attstr name="name" val="russia-stage1" />
      <attstr name="category" val="circuit" />
    </section>
  </section>
  <section name="Races">
    <section name="1">
      <attstr name="name" val="Practice" />
    </section>
  </section>
  <section name="Practice">
    <attnum name="laps" val="1" />
    <attstr name="penalties" val="no" />
    <attstr name="type" val="practice" />
    <attstr name="starting order" val="drivers list" />
    <attstr name="restart" val="yes" />
    <attstr name="display mode" val="normal" />
    <attstr name="display results" val="no" />
    <attnum name="distance" unit="km" val="0" />
    <attnum name="sessiontime" val="0" unit="s" />
    <attstr name="rain" val="none" />
    <attstr name="clouds" val="scarce clouds" />
    <attstr name="time of day" val="morning" />
    <attstr name="season" val="summer" />
    <attstr name="weather" val="config" />
    <section name="Starting Grid">
      <attnum name="rows" val="1" />
      <attnum name="distance to start" val="0.0" unit="m" />
      <attnum name="distance between columns" val="10" />
      <attnum name="offset within a column" val="0" />
      <attnum name="initial speed" val="0" unit="m/s" />
      <attnum name="initial height" val="0.30" unit="m" />
    </section>
  </section>
  <section name="Drivers">
    <attnum name="maximum number" val="1" />
    <attstr name="focused module" val="human" />
    <attstr name="rejected types" val="networkhuman" />
    <section name="1">
      <attstr name="driver name" val="Rally Driver" />
      <attstr name="module" val="human" />
      <attnum name="idx" val="1" />
      <attstr name="car name" val="ford-escort-rs2000" />
    </section>
  </section>
  <section name="Configuration">
    <attnum name="current configuration" val="4" />
    <section name="1">
      <attstr name="type" val="track select" />
    </section>
    <section name="2">
      <attstr name="type" val="drivers select" />
    </section>
    <section name="3">
      <attstr name="type" val="race config" />
      <attstr name="race" val="Practice" />
      <section name="Options">
        <section name="1">
          <attstr name="type" val="race length" />
        </section>
        <section name="2">
          <attstr name="type" val="display mode" />
        </section>
        <section name="3">
          <attstr name="type" val="time of day" />
        </section>
        <section name="4">
          <attstr name="type" val="cloud cover" />
        </section>
        <section name="5">
          <attstr name="type" val="rain fall" />
        </section>
        <section name="6">
          <attstr name="type" val="season" />
        </section>
        <section name="7">
          <attstr name="type" val="weather" />
        </section>
      </section>
    </section>
  </section>
</params>
"""
    share_raceman = SHARE / "config/raceman/rally-russia-stage1.xml"
    share_raceman.parent.mkdir(parents=True, exist_ok=True)
    share_raceman.write_text(raceman_content, encoding="utf-8")
    print(f"Creado: {share_raceman}")

def setup_user_settings():
    print(f"Configurando entorno aislado en {RALLY_SETTINGS}...")
    RALLY_SETTINGS.mkdir(parents=True, exist_ok=True)
    
    # 1. Copiar arbol desde initial-settings
    for item in INITIAL_SETTINGS.rglob("*"):
        rel = item.relative_to(INITIAL_SETTINGS)
        target = RALLY_SETTINGS / rel
        if item.is_dir():
            target.mkdir(parents=True, exist_ok=True)
        else:
            if not target.exists():
                shutil.copy2(item, target)

    # 2. Configurar rust_physics.xml para el Ford Escort
    rust_phys_content = """<?xml version="1.0" encoding="UTF-8"?>
<params name="Open Formula Rust physics">
  <section name="RustPhysics">
    <attstr name="library" val="modules/simu/vehicle_physics_engine.dll" />
    <attstr name="profile" val="cars/rust/ford-escort-rs2000/vehicle.json" />
    <attnum name="diagnostic telemetry" val="0" />
    <attnum name="diagnostic interval seconds" val="0.04" />
    <attnum name="diagnostic trace" val="0" />
    <attstr name="diagnostic trace directory" val="rustphysics/" />
    <attnum name="integration scenario" val="0" />
  </section>
</params>
"""
    (RALLY_SETTINGS / "config/rust_physics.xml").write_text(rust_phys_content, encoding="utf-8")
    print("Actualizado: config/rust_physics.xml")

    # 3. Configurar human.xml
    human_content = """<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE params SYSTEM "params.dtd">
<params name="human" version="1.1">
  <section name="Robots">
    <section name="index">
      <section name="1">
        <attstr name="name" val="Rally Driver"/>
        <attstr name="short name" val="Driver"/>
        <attstr name="code name" val="RAL"/>
        <attstr name="car name" val="ford-escort-rs2000"/>
        <attnum name="race number" val="1"/>
        <attnum name="red" val="0.2"/>
        <attnum name="green" val="0.4"/>
        <attnum name="blue" val="0.9"/>
        <attstr name="type" val="human"/>
        <attstr name="skill level" val="pro"/>
        <attstr name="nation" val="GB"/>
      </section>
    </section>
  </section>
</params>
"""
    (RALLY_SETTINGS / "drivers/human/human.xml").write_text(human_content, encoding="utf-8")
    print("Actualizado: drivers/human/human.xml")

    # 4. Asegurar racemanager en user-settings-rally
    raceman_rally = RALLY_SETTINGS / "config/raceman/rally-russia-stage1.xml"
    raceman_rally.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(SHARE / "config/raceman/rally-russia-stage1.xml", raceman_rally)
    print(f"Copiado racemanager a {raceman_rally}")

    # 5. Asegurar raceengine.xml usa simu rustphysics
    raceengine_xml = RALLY_SETTINGS / "config/raceengine.xml"
    if raceengine_xml.exists():
        txt = raceengine_xml.read_text(encoding="utf-8")
        if 'name="simu" val="rustphysics"' not in txt:
            # Reemplazar la linea de simu
            import re
            txt = re.sub(r'<attstr name="simu" val="[^"]*" />', '<attstr name="simu" val="rustphysics" />', txt)
            raceengine_xml.write_text(txt, encoding="utf-8")
            print("raceengine.xml asegurado con simu rustphysics")

    # 6. Copiar perfil rust ford-escort a user-settings si fuese necesario
    target_rust_profile = RALLY_SETTINGS / "cars/rust/ford-escort-rs2000"
    src_rust_profile = SHARE / "cars/rust/ford-escort-rs2000"
    if src_rust_profile.exists():
        if target_rust_profile.exists():
            shutil.rmtree(target_rust_profile)
        shutil.copytree(src_rust_profile, target_rust_profile)
        print("Perfil rust ford-escort-rs2000 sincronizado a user-settings-rally.")

if __name__ == '__main__':
    setup_raceman()
    setup_user_settings()
    print("Entorno de rally configurado correctamente.")
