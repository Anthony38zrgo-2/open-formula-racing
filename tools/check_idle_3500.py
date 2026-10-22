"""Etapa 6 (offline): afinacion del ralenti 3500 y cancelacion en mono.

1. Lee implementation/reports/audio_etapa2/01_idle_3500.wav (estereo 44100).
2. Goertzel en la banda de encendido V10 4T a 3500 RPM:
     firing = 3500/60*5 = 291.67 Hz, armonicos x2 (583.3) y x3 (875.0).
   Referencia: el mismo analisis sobre engine_idle_loop.wav del banco,
   cuya referencia acustica es 4531.6387 RPM -> firing 377.64 Hz.
   La relacion de fundamentales debe ser ~= 3500/4531.6387 (0.7723).
3. Cancelacion en mono: energia de (L+R)/2 frente a (L,R); informa la
   perdida en dB (cercana a 0 dB = sin cancelacion relevante).
4. Pico/RMS/clipping del segmento.
Sale con codigo 1 si algo queda fuera de tolerancia.
"""
import math
import struct
import sys
import wave

REPORT_WAV = "implementation/reports/audio_etapa2/01_idle_3500.wav"
BANK_IDLE = "sounds/banks/v10-v2-bank/engine_idle_loop.wav"

IDLE_REF_RPM = 4531.638723634397
PHYS_RPM = 3500.0
EXPECTED_RATIO = PHYS_RPM / IDLE_REF_RPM  # ~= 0.7723
FIRING_REF = IDLE_REF_RPM / 60.0 * 5.0    # 377.64 Hz en el loop
FIRING_3500 = PHYS_RPM / 60.0 * 5.0       # 291.67 Hz esperados


def read_mono(path, skip_s=0.5, take_s=1.0):
    with wave.open(path, "rb") as sample:
        assert sample.getnchannels() in (1, 2)
        assert sample.getframerate() == 44100
        assert sample.getsampwidth() == 2
        frames = sample.getnframes()
        channels = sample.getnchannels()
        start = int(skip_s * 44100)
        count = int(take_s * 44100)
        sample.setpos(start)
        raw = sample.readframes(count)
        values = struct.unpack("<%dh" % (count * channels), raw)
        left = [v / 32768.0 for v in values[0::channels]]
        right = [v / 32768.0 for v in values[1::channels]] if channels == 2 else list(left)
        return left, right, frames


def goertzel(samples, rate, target):
    n = len(samples)
    k = target * n / rate
    omega = 2.0 * math.pi * k / n
    cosine, sine = math.cos(omega), math.sin(omega)
    coeff = 2.0 * cosine
    s0 = s1 = s2 = 0.0
    for sample in samples:
        s0 = sample + coeff * s1 - s2
        s2, s1 = s1, s0
    power = s1 * s1 + s2 * s2 - coeff * s1 * s2
    return power / (n * n)


def spectrum_peak(samples, rate, lo, hi, step=0.5):
    best_f, best_p = lo, 0.0
    f = lo
    while f <= hi:
        power = goertzel(samples, rate, f)
        if power > best_p:
            best_f, best_p = f, power
        f += step
    return best_f, best_p


def main():
    left, right, frames = read_mono(REPORT_WAV)
    mono = [(a + b) * 0.5 for a, b in zip(left, right)]
    bank, _, _ = read_mono(BANK_IDLE)
    failures = []

    # 1. Fundamental del banco (referencia): debe estar junto a 377.6 Hz.
    bank_peak, _ = spectrum_peak(bank, 44100, 340.0, 420.0)
    print("bank idle fundamental: %.2f Hz (ref %.2f)" % (bank_peak, FIRING_REF))
    if abs(bank_peak - FIRING_REF) > 3.0:
        failures.append("bank fundamental off: %.2f" % bank_peak)

    # 2. Fundamental a 3500: debe estar junto a 291.7 Hz, ratio ~= 0.7723.
    peak, power = spectrum_peak(mono, 44100, 250.0, 340.0)
    ratio = peak / bank_peak
    print("3500 idle fundamental: %.2f Hz (esperado %.2f), ratio %.4f (esperado %.4f)"
          % (peak, FIRING_3500, ratio, EXPECTED_RATIO))
    if abs(peak - FIRING_3500) > 4.0:
        failures.append("3500 fundamental off: %.2f" % peak)
    if abs(ratio - EXPECTED_RATIO) > 0.012:
        failures.append("ratio off: %.4f" % ratio)
    # Armonicos audibles del encendido.
    for mult in (2, 3):
        harmonic, harmonic_power = spectrum_peak(mono, 44100, FIRING_3500 * mult - 12.0,
                                                 FIRING_3500 * mult + 12.0)
        audible = harmonic_power > power * 0.002
        print("armonico x%d: %.2f Hz energia_relativa=%.4f %s"
              % (mult, harmonic, harmonic_power / max(power, 1e-12),
                 "OK" if audible else "DEBIL"))
        if not audible:
            failures.append("harmonico x%d debil" % mult)

    # 3. Cancelacion en mono.
    energy = lambda xs: sum(x * x for x in xs) / len(xs)
    stereo_energy = (energy(left) + energy(right)) * 0.5
    mono_energy = energy(mono)
    loss_db = 10.0 * math.log10(stereo_energy / max(mono_energy, 1e-12))
    print("energia estereo %.6f mono %.6f perdida %.2f dB"
          % (stereo_energy, mono_energy, loss_db))
    if abs(loss_db) > 3.0:
        failures.append("cancelacion en mono: %.2f dB" % loss_db)

    # 4. Pico/RMS/clipping.
    peak_all = max(max(map(abs, left)), max(map(abs, right)))
    rms = math.sqrt(stereo_energy)
    print("pico %.3f rms %.4f %s"
          % (peak_all, rms, "CLIPPING" if peak_all >= 0.999 else "sin clipping"))
    if peak_all >= 0.999:
        failures.append("clipping")
    if peak_all < 0.005:
        failures.append("casi silencio")

    print("frames_wav:", frames)
    if failures:
        print("FALLOS:", "; ".join(failures))
        return 1
    print("RALENTI_3500_OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
