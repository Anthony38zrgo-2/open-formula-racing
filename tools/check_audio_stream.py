import os
import pathlib
import subprocess
import tempfile

import check_cxx_audio as compiler


def main():
    root = pathlib.Path(__file__).resolve().parent.parent
    msvc, executable = compiler.find_msvc()
    if executable is None:
        raise RuntimeError("MSVC is required for the OpenAL transport regression")
    library = root / "distribution/bin/libopenal-1.dll"
    audio = root / "rust/target/release/open_formula_audio.dll"
    with tempfile.TemporaryDirectory(prefix="ofp-stream-regression-") as directory:
        directory = pathlib.Path(directory)
        compiler.write_stubs(directory)
        environment = dict(os.environ)
        environment["INCLUDE"] = ";".join(map(str, [
            msvc / "include", compiler.SDK_INCLUDE / "ucrt",
            compiler.SDK_INCLUDE / "um", compiler.SDK_INCLUDE / "shared",
        ]))
        environment["LIB"] = ";".join(map(str, [
            msvc / "lib/x64", compiler.SDK_LIB / "um/x64", compiler.SDK_LIB / "ucrt/x64",
        ]))
        environment["PATH"] = str(executable.parent) + ";" + str(library.parent) + ";" + environment["PATH"]
        environment["ALSOFT_DRIVERS"] = "null"
        exports = [
            "alGetError", "alGenSources", "alDeleteSources", "alIsSource",
            "alGenBuffers", "alDeleteBuffers", "alIsBuffer", "alSourcef", "alSourcei",
            "alSource3f", "alSourcePlay", "alSourceStop", "alSourceUnqueueBuffers",
            "alGetSourcei", "alBufferData", "alSourceQueueBuffers", "alcOpenDevice",
            "alcCreateContext", "alcMakeContextCurrent", "alcDestroyContext", "alcCloseDevice",
        ]
        definition = directory / "openal.def"
        definition.write_text("LIBRARY libopenal-1.dll\nEXPORTS\n" + "\n".join(exports))
        import_library = directory / "openal.lib"
        subprocess.run([str(executable.parent / "lib.exe"), "/nologo", "/machine:x64",
                        "/def:" + str(definition), "/out:" + str(import_library)],
                       cwd=directory, env=environment, check=True, capture_output=True)
        command = [str(executable), "/nologo", "/EHsc", "/std:c++17", "/DNOMINMAX", "/DTRACE_LEVEL=0"]
        for folder in ["src/interfaces", "src/libs/tgf", "src/libs/math", "src/libs/portability",
                       "src/modules/sound/snddefault"]:
            command.extend(["/I", str(root / folder)])
        command.extend(["/I", str(directory)])
        for source in ["tools/check_audio_stream.cpp", "src/modules/sound/snddefault/ofp_v10_stream.cpp",
                       "src/modules/sound/snddefault/ofp_audio_loader.cpp"]:
            command.append(str(root / source))
        target = directory / "audio-stream-test.exe"
        command.extend([str(import_library), "/Fe" + str(target)])
        result = subprocess.run(command, cwd=directory, env=environment, capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        subprocess.run([str(target), str(audio), str(root / "sounds")], cwd=directory,
                       env=environment, check=True, timeout=60)


if __name__ == "__main__":
    main()
