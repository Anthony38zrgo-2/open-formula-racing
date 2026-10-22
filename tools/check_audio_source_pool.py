import argparse
import os
import pathlib
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=pathlib.Path, required=True)
    build = parser.parse_args().build_dir.resolve()
    root = pathlib.Path(__file__).resolve().parent.parent
    cache = {}
    for line in (build / "CMakeCache.txt").read_text().splitlines():
        if "=" in line and not line.startswith(("#", "//")):
            name, value = line.split("=", 1)
            cache[name.split(":", 1)[0]] = value
    compiler = pathlib.Path(cache["CMAKE_CXX_COMPILER"])
    native = compiler.parent.parent
    dependencies = build.parent / "dependencies"
    environment = dict(os.environ)
    environment["PATH"] = str(build / "bin") + ";" + str(compiler.parent) + ";" + environment["PATH"]
    environment["ALSOFT_DRIVERS"] = "null"
    with tempfile.TemporaryDirectory(prefix="ofp-source-pool-") as directory:
        target = pathlib.Path(directory) / "source-pool-test.exe"
        command = [str(compiler), "-std=c++17", "-DTRACE_LEVEL=0", "-DSNDDEFAULT_DLL",
                   "-o", str(target), str(root / "tools/check_audio_source_pool.cpp")]
        for folder in [root / "src/interfaces", root / "src/libs/tgf", root / "src/libs/math",
                       root / "src/libs/portability", root / "src/modules/sound/snddefault",
                       dependencies / "include", native / "include", native / "include/SDL2", build]:
            command.extend(["-I", str(folder)])
        for unit in ["CarSoundData", "Sound", "OpenalSound", "SoundInterface", "OpenalSoundInterface"]:
            command.append(str(build / f"src/modules/sound/snddefault/CMakeFiles/snddefault.dir/{unit}.cpp.obj"))
        command.extend(map(str, [dependencies / "lib/libplibsg.a", dependencies / "lib/libplibul.a",
                                build / "src/libs/tgf/libtgf.dll.a", build / "src/libs/portability/libportability.dll.a"]))
        command.extend(["-L" + str(native / "lib"), "-lSDL2", "-lopenal", "-lwinmm", "-lopengl32", "-lglu32"])
        subprocess.run(command, cwd=directory, env=environment, check=True)
        subprocess.run([str(target)], cwd=directory, env=environment, check=True, timeout=30)


if __name__ == "__main__":
    main()
