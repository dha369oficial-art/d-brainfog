@echo off
REM Requisitos: Visual Studio 2022 (carga de trabajo "Desarrollo para el escritorio con C++"), CMake 3.22+ y Git.
REM Se necesita conexion a internet la primera vez (descarga JUCE 8.0.4).
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 || goto :err
cmake --build build --config Release --target dBrainFog_VST3 --parallel || goto :err
echo.
echo OK: build\dBrainFog_artefacts\Release\VST3\d-BrainFog.vst3
echo Copia esa carpeta a  C:\Program Files\Common Files\VST3\
goto :eof
:err
echo La compilacion fallo. Revisa el mensaje de arriba.
exit /b 1
