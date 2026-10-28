# Informe de pruebas — motor DSP de d-BrainFog 1.0

Ejecutado en: g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, Linux x86_64, 2026-10-28
Comando: g++ -std=c++17 -O2 -Wall -Wextra tests/dsp_test.cpp  (también con -fsanitize=address,undefined: 0 errores)

```
[1] tabla de 50 parametros: ids unicos, rangos validos
[2] presets x sample rate x tamano de bloque (finitos, sin clipping, sin clic en inicio)
[3] ausencia de clics en inicio/fin de nota
    After Dark   1er sample=0.0000  maxDelta sost=0.0273 rel=0.0193  cola final rms=0.000001
    Cold Hall    1er sample=0.0000  maxDelta sost=0.0098 rel=0.0063  cola final rms=0.000205
    Broken Tape  1er sample=0.0000  maxDelta sost=0.0411 rel=0.0172  cola final rms=0.000004
    Red Static   1er sample=0.0004  maxDelta sost=0.1130 rel=0.0753  cola final rms=0.000004
[4] presets distintos (RMS / brillo)
    After Dark   rms=0.0772 brillo=0.0631
    Cold Hall    rms=0.0831 brillo=0.0766
    Broken Tape  rms=0.0945 brillo=0.0426
    Red Static   rms=0.2101 brillo=0.0662
[5] resultado identico con bloques 64 vs 1024
    After Dark   max|diff|=0.00e+00
    Cold Hall    max|diff|=0.00e+00
    Broken Tape  max|diff|=0.00e+00
    Red Static   max|diff|=0.00e+00
[6] afinacion (La4=440 Hz) y pitch bend
    La4: 440.00 Hz
    Pitch bend +2 st: 494.00 Hz (esperado 493.88)
    velocity: rms(0.2)=0.0741 rms(1.0)=0.1543
[7] polifonia, voice stealing, legato, pedal de sustain
    20 notas con 4 voces: pico=0.379 maxDelta=0.090
    8 voces simultaneas: pico=0.516
    legato+glide: pico=0.165
    sustain: rms con pedal=0.0578  tras soltar=0.00161
[8] parametros extremos y aleatorios (incluye NaN/inf/fuera de rango)
    120 configuraciones: 0 con problemas
    parametros sin efecto audible: 0
[9] asignaciones de memoria durante process()/MIDI
    asignaciones: 0
[10] CPU con 8 voces + 4 efectos: 4.5 % de un nucleo (esta maquina Linux, -O2)

RESULTADO: 224 comprobaciones OK, 0 fallos
```

## NO realizado (no era posible en el entorno de generación)
- Compilación del `.vst3` con JUCE (sin red para descargar JUCE, sin MSVC/MinGW). **No hay binario.**
- Verificación de 64 bits del binario, carga en FL Studio, automatización, guardado/reapertura de proyecto en el DAW.
- Compilación de `PluginProcessor.*` y `PluginEditor.*` (pueden contener errores de sintaxis/API de JUCE aún sin detectar).
- Prueba visual de la interfaz.
- Compilación de los tests con MSVC (el workflow de GitHub Actions los ejecuta en Windows).
Lo probado aquí es únicamente la lógica de audio (`Source/DSP/*`), que es independiente de JUCE.
