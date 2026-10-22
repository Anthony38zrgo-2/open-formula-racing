#!/usr/bin/env python3
"""Puerta C++ del audio V10 (Etapas 3-4): sintaxis MSVC + test del drenaje.

Verifica con `cl /Zs` (sin enlazar) todos los TUs nuevos/tocados contra las
cabeceras reales del proyecto, con stubs SOLO para terceros (OpenAL/PLIB +
`config.h` generado por CMake) escritos en un directorio temporal — jamas en
el repo — y compila+ejecuta `tools/check_audio_drain.cpp`. Sin MSVC instalado
la puerta se salta con SKIP (exit 0) e informa del motivo.

Usage: python tools/check_cxx_audio.py
"""

import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent

MSVC_CANDIDATES = [
    pathlib.Path("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207"),
]
SDK_INCLUDE = pathlib.Path("C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0")
SDK_LIB = pathlib.Path("C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0")

AL_H = """\
/* Minimal OpenAL al.h stub for syntax checking (real values in OpenAL). */
#ifndef AL_H_STUB
#define AL_H_STUB
typedef unsigned int ALuint;
typedef int ALint;
typedef int ALenum;
typedef float ALfloat;
typedef int ALsizei;
typedef void ALvoid;
#define AL_NO_ERROR 0
#define AL_TRUE 1
#define AL_FALSE 0
#define AL_POSITION 0x1004
#define AL_VELOCITY 0x1006
#define AL_PITCH 0x1003
#define AL_GAIN 0x100A
#define AL_REFERENCE_DISTANCE 0x1020
#define AL_ROLLOFF_FACTOR 0x1021
#define AL_MAX_DISTANCE 0x1023
#define AL_SOURCE_RELATIVE 0x202
#define AL_BUFFERS_PROCESSED 0x1016
#define AL_BUFFERS_QUEUED 0x1015
#define AL_SOURCE_STATE 0x1010
#define AL_PLAYING 0x1012
#define AL_FORMAT_MONO16 0x1101
#define AL_FORMAT_STEREO16 0x1103
#define AL_VENDOR 0xB001
#define AL_RENDERER 0xB002
#define AL_VERSION 0xB003
#define AL_INVERSE_DISTANCE 0xD001
#define AL_ORIENTATION 0x100F
#ifdef __cplusplus
extern "C" {
#endif
int alGetError(void);
void alGenSources(int n, ALuint* sources);
void alDeleteSources(int n, const ALuint* sources);
int alIsSource(ALuint source);
void alGenBuffers(int n, ALuint* buffers);
void alDeleteBuffers(int n, const ALuint* buffers);
int alIsBuffer(ALuint buffer);
void alSourcef(ALuint source, int param, float value);
void alSourcei(ALuint source, int param, int value);
void alSource3f(ALuint source, int param, float v1, float v2, float v3);
void alSourcefv(ALuint source, int param, const float* values);
void alSourcePlay(ALuint source);
void alSourceStop(ALuint source);
void alSourcePause(ALuint source);
void alSourceQueueBuffers(ALuint source, int n, const ALuint* buffers);
void alSourceUnqueueBuffers(ALuint source, int n, ALuint* buffers);
void alGetSourcei(ALuint source, int param, ALint* value);
void alBufferData(ALuint buffer, int format, const void* data, int size, int freq);
const char* alGetString(int param);
void alDistanceModel(int model);
void alDopplerFactor(float v);
void alDopplerVelocity(float v);
void alListenerfv(int param, const float* v);
void alListenerf(int param, float v);
#ifdef __cplusplus
}
#endif
#endif
"""

ALC_H = """\
/* Minimal OpenAL alc.h stub for syntax checking. */
#ifndef ALC_H_STUB
#define ALC_H_STUB
typedef int ALCint;
typedef int ALCenum;
typedef struct ALCdevice_struct ALCdevice;
typedef struct ALCcontext_struct ALCcontext;
ALCcontext* alcGetCurrentContext(void);
ALCdevice* alcOpenDevice(const char* name);
ALCcontext* alcCreateContext(ALCdevice* dev, const int* attr);
int alcCloseDevice(ALCdevice* dev);
int alcMakeContextCurrent(ALCcontext* ctx);
void alcDestroyContext(ALCcontext* ctx);
int alcGetError(ALCdevice* dev);
const char* alcGetString(ALCdevice* dev, int param);
#endif
"""

SG_H = """\
/* Minimal PLIB sg.h stub for syntax checking (real values in PLIB). */
#ifndef SG_H_STUB
#define SG_H_STUB
typedef float sgVec2[2];
typedef float sgVec3[3];
typedef float sgVec4[4];
typedef float sgMat4[4][4];
void sgMakeIdentMat4(sgMat4 m);
void sgInvertMat4(sgMat4 dst, const sgMat4 src);
void sgSetVec3(sgVec3 dst, float x, float y, float z);
void sgXformPnt3(sgVec3 dst, const sgVec3 src, const sgMat4 m);
void sgXformPnt3(sgVec3 dst, const sgMat4 m);
void sgMakeCoordMat4(sgMat4 m, float x, float y, float z, float h, float p, float r);
#endif
"""

SL_H = """\
/* Minimal PLIB sl.h stub for syntax checking. */
#ifndef SL_H_STUB
#define SL_H_STUB
class slSample;
class slEnvelope;
class slScheduler;
#define SL_SAMPLE_ONE_SHOT 0
extern char *__slPendingError;
#endif
"""

CONFIG_H = """\
/* Empty config.h stub for syntax checking (real one is CMake-generated). */
#ifndef CONFIG_H_STUB
#define CONFIG_H_STUB
#endif
"""


def find_msvc():
    for root in MSVC_CANDIDATES:
        cl = root / "bin" / "Hostx64" / "x64" / "cl.exe"
        if cl.is_file():
            return root, cl
    return None, None


def write_stubs(stub_dir):
    (stub_dir / "AL").mkdir(parents=True, exist_ok=True)
    (stub_dir / "plib").mkdir(parents=True, exist_ok=True)
    (stub_dir / "AL" / "al.h").write_text(AL_H)
    (stub_dir / "AL" / "alc.h").write_text(ALC_H)
    (stub_dir / "plib" / "sg.h").write_text(SG_H)
    (stub_dir / "plib" / "sl.h").write_text(SL_H)
    (stub_dir / "config.h").write_text(CONFIG_H)


def run(cmd, env):
    completed = subprocess.run(cmd, capture_output=True, text=True, cwd=str(REPO_ROOT), env=env)
    return completed.returncode, (completed.stdout + completed.stderr)[-3000:]


def main():
    msvc_root, cl = find_msvc()
    if cl is None or not SDK_INCLUDE.is_dir():
        print("SKIP: no MSVC/SDK toolchain installed (CXX gate needs cl.exe)")
        return 0
    stub_dir = pathlib.Path(tempfile.mkdtemp(prefix="ofp-cxx-stubs-"))
    work_dir = pathlib.Path(tempfile.mkdtemp(prefix="ofp-cxx-work-"))
    failures = 0
    try:
        write_stubs(stub_dir)
        env = dict(os.environ)
        env["INCLUDE"] = ";".join([
            str(msvc_root / "include"),
            str(SDK_INCLUDE / "ucrt"), str(SDK_INCLUDE / "um"), str(SDK_INCLUDE / "shared"),
        ])
        base_includes = [
            "/I", "src/interfaces", "/I", "src/libs/tgf", "/I", "src/libs/math",
            "/I", "src/libs/robottools", "/I", "src/libs/portability",
            "/I", "src/modules/sound/snddefault", "/I", "src/modules/simu/rustphysics",
            "/I", str(stub_dir),
        ]
        common = ["/nologo", "/Zs", "/TP", "/std:c++17", "/EHsc", "/DNOMINMAX",
                  "/DTRACE_LEVEL=0", "/DTRACE_OUT", "/DWIN32", "/FIcassert", "/FIalgorithm"]

        def check_tu(label, tu, extra=()):
            nonlocal failures
            wrapper = work_dir / (label + "_check.cpp")
            wrapper.write_text(f'#include "{tu}"\nint ofp_gate_dummy;\n')
            code, out = run([str(cl)] + common + list(extra) + base_includes + [str(wrapper)], env)
            status = "PASS" if code == 0 else "FAIL"
            print(f"{status}: {label}")
            if code != 0:
                failures += 1
                print(out[-1500:])

        check_tu("abi_headers", "ofp_audio_abi.h", ("/I", "src/interfaces"))
        check_tu("rust_abi", "rust_abi.h", ("/I", "src/modules/simu/rustphysics"))
        check_tu("loader", "ofp_audio_loader.cpp")
        check_tu("stream", "ofp_v10_stream.cpp")
        check_tu("grsound", "grsound.cpp", ("/DSNDDEFAULT_DLL",))
        check_tu("carsound", "CarSoundData.cpp")
        check_tu("oal", "OpenalSoundInterface.cpp")
        check_tu("rustphysics", "rustphysics.cpp", ("/DRUSTPHYSICS_DLL",))
        check_tu("rust_library", "rust_library.cpp", ("/DRUSTPHYSICS_DLL",))

        # Build + run the drain test (needs link).
        env["LIB"] = ";".join([
            str(msvc_root / "lib" / "x64"),
            str(SDK_LIB / "um" / "x64"), str(SDK_LIB / "ucrt" / "x64"),
        ])
        env["PATH"] = str(msvc_root / "bin" / "Hostx64" / "x64") + ";" + env.get("PATH", "")
        drain_src = REPO_ROOT / "tools" / "check_audio_drain.cpp"
        drain_exe = work_dir / "ofp-drain-test.exe"
        code, out = run(
            [str(cl), "/nologo", "/EHsc", "/std:c++17",
             "/I", "src/interfaces",
             "/Fo" + str(work_dir / "audio-drain.obj"),
             "/Fe" + str(drain_exe), str(drain_src)], env)
        if code != 0:
            failures += 1
            print("FAIL: drain build")
            print(out[-1500:])
        else:
            completed = subprocess.run([str(drain_exe)], capture_output=True, text=True)
            if completed.returncode == 0 and "OFP_DRAIN_TEST_OK" in completed.stdout:
                print("PASS: drain run")
            else:
                failures += 1
                print("FAIL: drain run")
                print((completed.stdout + completed.stderr)[-1500:])
    finally:
        shutil.rmtree(stub_dir, ignore_errors=True)
        shutil.rmtree(work_dir, ignore_errors=True)
    if failures:
        print(f"CXX_GATE_FAILURES: {failures}")
        return 1
    print("CXX_GATE_OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
