# DEVELOPMENT_STATE — MiniSO Lab

## Fase actual

Fase 1 (núcleo + consola) en implementación. Fase 2 (Qt) no iniciada.

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

## Cómo continuar

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
./build/miniso --demo
```

No reconstruir el núcleo para Qt. Añadir `src/gui` y un ejecutable `miniso_gui` que enlace `miniso_core`.
