# DEVELOPMENT_STATE — MiniSO Lab

## Fase actual

Fase 1 (núcleo + consola) validada. Resultado: 9/9 suites CTest aprobadas.
Fase 2 (Qt Widgets) no iniciada y debe comenzar únicamente sobre esta versión.

## Estado de aceptación

- Compilación C++17 con CMake: aprobada.
- Procesos simulados, estados, BCP y árbol: implementados y probados.
- Round Robin: implementado y probado.
- First-Fit, fragmentación externa y liberación/coalescing: probados.
- Paginación, traducción, fragmentación interna y swap FIFO: probados.
- OOM simulado: probado.
- Productor-consumidor POSIX real: probado.
- MiniRenderer POSIX real: probado mediante demo y suite de integración.
- `fork()`/`exec()`/`waitpid()` reales: probados mediante el auxiliar.
- Cinco aplicaciones simuladas: implementadas desde la API y demostradas por integración.
- Escenarios A–D: ejecutables desde consola y demo.
- Configuración JSON: cargada al iniciar la consola con validación de rangos.

## Correcciones técnicas relevantes

- Se corrigió la detección de fragmentación externa para una solicitud mayor que todos los huecos disponibles.
- Se corrigió el cierre parcial de MiniRenderer para no hacer `pthread_join()` sobre identificadores no creados.
- Se añadió una prueba de carga y rechazo de configuración JSON.

## Evidencias

- `docs/evidence/phase1/ctest.txt`
- `docs/evidence/phase1/demo.txt`
- `docs/evidence/phase1/linux.txt`

## Arquitectura estable para Qt

- Fachada `miniso::MiniSO` como única API de aplicación.
- Núcleo sin Qt mediante la biblioteca estática `miniso_core`.
- La consola llama exclusivamente a esa fachada.
- PID simulados, 1–6 reservados y usuarios desde 100; nunca se reutilizan dentro de una sesión.
- `process_runner` es el único proceso auxiliar real de Fase 1.
- Productor-consumidor y MiniRenderer usan POSIX Threads reales.
- El sistema de eventos usa snapshot y sincronización por mutex.

## Validación

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
./build/miniso --demo
./build/process_runner --demo --child ./build/process_child
```

## Continuación fase 2

No reconstruir ni duplicar los algoritmos del núcleo.
Añadir `src/gui`, un ejecutable Qt y una capa de servicios que exponga operaciones de `MiniSO` a widgets Qt.
Antes de implementar Qt, conservar esta versión y la salida de las pruebas como punto de partida.
