# d-BrainFog v1.0 — por DHA369

Sintetizador VST3 (instrumento) de estética post-punk / darkwave / coldwave. Proyecto JUCE 8 + C++17.

> **Estado honesto:** el motor de audio (`Source/DSP`) está probado. El código de la interfaz y del
> envoltorio VST3 (`PluginProcessor.*`, `PluginEditor.*`, `CMakeLists.txt`) está escrito pero **todavía no se ha
> compilado**: el entorno donde se generó no tenía JUCE, red ni compilador de Windows. **No existe aún un `.vst3` compilado.**
> Ver "Evidencia" más abajo.

## Cómo obtener el .vst3 (elige una vía)

### A) Sin instalar nada: GitHub Actions (recomendada)
1. Crea un repositorio privado en GitHub y sube el contenido de esta carpeta (incluida `.github/`).
2. Pestaña **Actions → Build d-BrainFog (Windows x64 VST3) → Run workflow**.
3. Al terminar (~10 min) descarga el artefacto `d-BrainFog-vst3-win64`: contiene la carpeta `d-BrainFog.vst3`.
   El flujo también ejecuta las pruebas DSP y verifica con `dumpbin` que el binario es x64.

### B) En tu PC con Windows
Requisitos: Visual Studio 2022 (carga "Desarrollo para el escritorio con C++"), CMake ≥ 3.22, Git, internet.
Ejecuta `build_windows.bat`. Resultado: `build\dBrainFog_artefacts\Release\VST3\d-BrainFog.vst3`.

> Si el compilador muestra errores en `PluginEditor.cpp` o `PluginProcessor.cpp`, envíame el mensaje: es la parte aún no compilada.

## Instalación en FL Studio (64 bits)
1. Copia la **carpeta** `d-BrainFog.vst3` a `C:\Program Files\Common Files\VST3\`.
2. FL Studio → *Options → Manage plugins* → **Find installed plugins** (con "Verify plugins" activado).
3. Abre el *Plugin picker* → *Installed → Generators → VST3 → d-BrainFog*. Se carga como instrumento.
4. Abre el Piano roll / conecta un teclado MIDI. Automatización: clic derecho en cualquier mando → *Create automation clip*.
5. Presets de usuario: botones **Save / Load** (carpeta por defecto `Documentos\DHA369\d-BrainFog\Presets`).
   Los 4 presets de fábrica están en el selector y también como archivos en `Presets/*.dbfpreset`
   (cópialos a esa carpeta y usa **Load**). El estado se guarda dentro del proyecto de FL Studio.

## Uso rápido
- **Macros**: Tone (oscuro↔brillante), Cutoff, Grit (saturación), Drift (inestabilidad de afinación), Dark Space (oscuridad de la reverb), Space Mix (cantidad de reverb).
- **Osciladores**: 2 osc (saw/pulse/triángulo) + sub + ruido, Detune, PW. **Filtro**: paso bajo resonante 12 dB/oct, envolvente y keytrack.
- **LFO**: seno/triángulo/cuadrada/S&H → tono, cutoff, ancho de pulso; con **Sync** al tempo del host.
- **Voice**: polifonía 1–8, Legato (monofónico con portamento), Glide, rango de pitch bend. Soporta velocity, pitch bend y pedal de sustain (CC64).
- **Efectos** (cada uno con On/Off): distorsión, chorus estéreo, delay (ping-pong con feedback oscurecido), reverb oscura. Salida con limitador suave anti-clipping.

## Pruebas del motor DSP
`./tests/run_tests.sh` (Linux/macOS) o `cmake -DBUILD_DSP_TESTS=ON`. Genera además WAV de demostración: `./tests/run_tests.sh --demos demos`.

## Licencia de JUCE
JUCE 8 se distribuye bajo AGPLv3 o licencia comercial. Para uso personal o código abierto basta AGPLv3; si vas a vender o distribuir el plugin sin código fuente, necesitas una licencia JUCE comercial.

## Limitaciones conocidas
- Filtro de 12 dB/oct (no 24). La distorsión no usa sobremuestreo (algo de aliasing, acorde al carácter lo-fi).
- Modwheel (CC1) no está asignado. El editor es de tamaño fijo (1000×712).
