# Stereo Tool Pro (VST3, Windows x64)

Plugin VST3 inspirado en Flux Stereo Tool, hecho con JUCE.

- Ganancia de entrada L/R independiente (-inf a +24 dB, pasos de 0.01 dB)
- Pan individual de cada canal (L y R) + Width + Pan general
- Inversion de fase (polaridad) por canal
- Vectorscopio (L/R rotado a M/S)
- Medidores PPM: L in, R in, L out, R out (con pico retenido)
- Correlacion de fase continua (-1 a +1)

## Compilar con GitHub
1. Sube TODO el contenido de esta carpeta a la RAIZ de un repo (debe verse `CMakeLists.txt`, `Source/` y `.github/` en la raiz).
2. Pestana **Actions** > "Build VST3 (Windows x64)" > se ejecuta solo al subir (o Run workflow).
3. Al terminar (verde), entra a la ejecucion y descarga el artefacto `StereoToolPro-VST3-Windows-x64`.
4. Descomprime y copia la carpeta `Stereo Tool Pro.vst3` a `C:\Program Files\Common Files\VST3`.
5. En FL Studio: Options > Manage plugins > Find installed plugins.

## Uso
- Doble clic en una perilla = valor por defecto.
- Clic en el numero = escribe el valor exacto (ej. `-3.25`, `L 40`, `150`).
- Arrastre lento = ajuste fino.
