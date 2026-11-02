# DEVELOPMENT_STATE — MiniSO Lab

## Fase actual

Fase 1 (núcleo + consola) validada mediante compilación y CTest. Fase 2 (Qt) no iniciada.

## Decisiones

- Fachada `miniso::MiniSO` como única API de aplicación.
- Núcleo sin Qt. Biblioteca estática `miniso_core`.
- PID usuario desde 100; sistema 1–6; no se reutilizan.
- Hijos reparentados a PID 1 salvo cierre en cascada de aplicaciones.
- Particionamiento y paginación son modelos independientes que MiniSO actualiza al crear/terminar procesos.
- Swap solo almacena páginas expulsadas, no todas las páginas virtuales.
- Capacidad de respaldo = marcos libres + slots de swap libres. Si no alcanza, se dispara OOM simulado.
- Process runner es un ejecutable separado; MiniSO lo lanza y lee JSON por stdout.
- Productor-consumidor usa pthreads, mutex y cond reales.
- La creación de procesos asigna primero memoria de partición y paginación; si alguna falla, se revierte y el proceso queda terminado sin memoria.
- La recuperación de páginas libera el slot de swap correspondiente al completar la carga.

## Estado de aceptación de Fase 1

Validado en la rama `fix/correciones-de-codigo`:

- Implementado y probado: compilación C++17/CMake, arranque, procesos 1–6, PID/PPID desde 100, árbol padre-hijo, terminación, Round Robin, particiones First-Fit, fusión, fragmentación externa, paginación, traducción, fragmentación interna, swap FIFO, recuperación de fallos, POSIX threads, proceso Linux auxiliar, OOM simulado, escenarios A–D, eventos exportables y consola.
- Pruebas: 8 suites CTest/GoogleTest; 100% aprobadas.
- Demo: `./build/miniso --demo` verifica procesos, pestañas, PageFault/MiniRenderer, MiniStress, productor-consumidor y `fork`/`exec`/`waitpid`.
- Próxima fase: no iniciar Qt hasta que exista autorización explícita.

## Cómo continuar

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
./build/miniso --demo
```

No reconstruir el núcleo para Qt. Añadir `src/gui` y un ejecutable `miniso_gui` que enlace `miniso_core`.
