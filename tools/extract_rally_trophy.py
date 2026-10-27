"""
Script de extraccion para pistas y recursos de Rally Trophy (.bzf).
Compatible con Rally Trophy (Bugbear Entertainment, 2001).
Permite extraer modelos 3D (.BMF), texturas (.TGA), notas de copiloto (.NPF),
trazadas de IA (.APF) y configuraciones (.INI).
"""

import os
import sys
import struct
import zlib
import argparse

# Clave XOR de 256 bytes de Bugbear bbzf v2001.06.06 (Rally Trophy)
RT_BZF_KEY = bytes.fromhex(
    "8bc2698046421825805daa026d491194a801e758cb8da6303e831b094cce18bb"
    "7cae6a301e000ac924c098fd33cc0444faea8f33c30df2e847d4dca8c754a2c7"
    "581732c87f644ac6fdc1e713ecb7ff1e0bd5729a57f83fdcdbc9f385c8866862"
    "c806ab9090f73ee4b46b8089c09369eb84c97ad4afd4f4d5a06849e878edd053"
    "7481bcd07a404feaf1c34baad6e6aa730ccf8e2bf42c7b9b3ebbc419ff104364"
    "03934dcf64c8e5a25bd330bd573b2a7e4cee96e54e853bf65eca4d5f84772a5b"
    "1e4147d6771569d29ba218096a1750d4ec2e7b4be4699caea89bcd042fabeb01"
    "6c95902ddab141435938e9d73804863c939723a5df5f068ac3392a4e2933f01e"
)

def list_bzf(archive_path):
    """Devuelve la lista de archivos contenidos en un archivo .bzf."""
    with open(archive_path, "rb") as f:
        magic = f.read(4)
        if magic != b"bbzf":
            raise ValueError(f"Fichero no valido o no es formato Bugbear bbzf: {magic}")
        version = f.read(4)
        cnt = struct.unpack("<I", f.read(4))[0]
        hdr = bytearray(f.read(cnt * 53))

    for i in range(len(hdr)):
        hdr[i] ^= RT_BZF_KEY[i % 256]

    file_list = []
    for i in range(cnt):
        entry = hdr[i*53 : (i+1)*53]
        flags = entry[0]
        data_off, unpacked_sz, packed_sz = struct.unpack("<III", entry[1:13])
        name = entry[13:53].split(b"\x00")[0].decode("latin1")
        file_list.append({
            "name": name,
            "flags": flags,
            "offset": data_off,
            "unpacked_size": unpacked_sz,
            "packed_size": packed_sz,
            "compressed": bool(flags & 1)
        })
    return file_list

def extract_bzf(archive_path, output_dir, filter_str=None):
    """Extrae los ficheros de un archivo .bzf."""
    os.makedirs(output_dir, exist_ok=True)
    with open(archive_path, "rb") as f:
        raw = f.read()

    files = list_bzf(archive_path)
    extracted = 0

    for item in files:
        fn = item["name"]
        if filter_str and (filter_str.lower() not in fn.lower()):
            continue

        off = item["offset"]
        psz = item["packed_size"]
        flg = item["flags"]

        file_enc = raw[off : off + psz]
        file_dec = bytes([b ^ RT_BZF_KEY[j % 256] for j, b in enumerate(file_enc)])

        if flg & 1:
            try:
                data = zlib.decompress(file_dec)
            except Exception as e:
                print(f"[ERROR] Fallo al descomprimir {fn}: {e}")
                continue
        else:
            data = file_dec

        # Los archivos comprimidos de JoWooD anteponen 43 bytes de copyright dentro del stream zlib
        jowood_prefix = b"(c) 2001 by JoWooD Productions Software AG\x00"
        if data.startswith(jowood_prefix):
            data = data[len(jowood_prefix):]

        out_path = os.path.join(output_dir, fn)
        with open(out_path, "wb") as out:
            out.write(data)
        extracted += 1
        print(f"  [OK] {fn} ({len(data)} bytes)")

    return extracted

def extract_track(rt_root, track_num, output_dir, stage_filter=None):
    """Extrae completamente una pista de Rally Trophy (data, models, textures)."""
    track_dir = os.path.join(rt_root, "Program Files", "JoWooD", "Rally Trophy", "Data", "Tracks", f"track{track_num}")
    if not os.path.isdir(track_dir):
        print(f"[ERROR] No se encontro el directorio de la pista: {track_dir}")
        return

    track_out = os.path.join(output_dir, f"track{track_num}")
    print(f"=== Extrayendo Pista {track_num} a {track_out} ===")

    for bzf_name in ["data.bzf", "models.bzf", "textures.bzf"]:
        bzf_path = os.path.join(track_dir, bzf_name)
        if os.path.exists(bzf_path):
            cat_out = os.path.join(track_out, bzf_name.replace(".bzf", ""))
            print(f"-> Extrayendo {bzf_name}...")
            count = extract_bzf(bzf_path, cat_out, filter_str=stage_filter)
            print(f"   Archivos extraidos: {count}")

def main():
    parser = argparse.ArgumentParser(description="Extractor de pistas y paquetes Rally Trophy (.bzf)")
    parser.add_argument("--game-dir", default=r"D:\games\rally-throphy", help="Ruta raiz del juego Rally Trophy")
    parser.add_argument("--track", type=int, choices=[1, 2, 3, 4, 5], help="Numero de pista (1=Rusia, 2=Suecia, 3=Finlandia, 4=Kenia, 5=Suiza)")
    parser.add_argument("--stage", type=str, default=None, help="Filtro opcional por tramo (ej. stage1)")
    parser.add_argument("--output", default=r"D:\open-formula\extracted_rt_tracks", help="Directorio destino de extraccion")
    parser.add_argument("--file", type=str, default=None, help="Extraer un unico archivo .bzf especifico")

    args = parser.parse_args()

    if args.file:
        print(f"Extrayendo archivo directo: {args.file}")
        extract_bzf(args.file, args.output, args.stage)
    elif args.track:
        extract_track(args.game_dir, args.track, args.output, args.stage)
    else:
        print("Uso: especifica --track [1-5] o --file [ruta.bzf].")
        print("Pistas disponibles en Rally Trophy:")
        print("  Track 1: Rusia (10 tramos - Asfalto, barro, grava)")
        print("  Track 2: Suecia (8 tramos - Nieve y hielo)")
        print("  Track 3: Finlandia (8 tramos - Grava rapida y saltos)")
        print("  Track 4: Kenia (8 tramos - Pistas de tierra polvorientas)")
        print("  Track 5: Suiza (8 tramos - Asfalto de montana)")

if __name__ == "__main__":
    main()
