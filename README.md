# MiniSO Lab — Interactive Operating System Simulator

Simulador educativo de sistemas operativos para Linux. La **Fase 1** entrega un núcleo en C++17 independiente de Qt y una consola interactiva. La Fase 2 (Qt Widgets) reutilizará exactamente este núcleo.

MiniSO **no es un kernel**. Distingue tres niveles:

- **Nivel A:** procesos, BCP, Round Robin, particionamiento First-Fit, paginación, swap FIFO y OOM simulados.
- **Nivel B:** `fork`/`exec`/`waitpid` reales (auxiliar `process_runner`) y POSIX threads reales (productor-consumidor y MiniRenderer).
- **Nivel C:** interfaz. En esta fase, consola. Qt no forma parte del núcleo.

## Características de la Fase 1

- Seis procesos de sistema al arrancar (PID 1–6).
- Creación manual de procesos, árbol padre-hijo y máquina de estados.
- Planificador Round Robin con reloj simulado.
- Memoria particionada First-Fit con fusión de huecos.
- Paginación, traducción de direcciones, fragmentación interna, swap FIFO y fallos de página.
- OOM simulado (no invoca el OOM killer de Linux).
- Productor-consumidor con `pthread_mutex_t` y `pthread_cond_t`.
- Cinco aplicaciones lógicas de escritorio y escenarios A–D.
- Registro centralizado de eventos.

## Requisitos

- Linux
- g++ con C++17
- CMake ≥ 3.16
- pthread
- Acceso de red la primera vez que se descargue GoogleTest (pruebas)

## Compilación

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

Sanitizers:

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DMINISO_ENABLE_SANITIZERS=ON
cmake --build build-asan -j$(nproc)
```

## Ejecución

```bash
./build/miniso
./build/miniso --demo
./build/process_runner --demo --child ./build/process_child
```

## Estructura

- `src/core` — MiniSO, eventos, configuración
- `src/processes` — BCP y administrador de procesos
- `src/scheduler` — Round Robin
- `src/memory` — particiones, paginación, swap, OOM
- `src/concurrency` — pthreads
- `src/linux` — cliente del auxiliar real
- `src/applications` — aplicaciones lógicas
- `src/console` — menú de terminal
- `process_runner` — `fork`/`exec`/`waitpid`
- `tests` — CTest + GoogleTest
- `docs` — arquitectura, algoritmos y estado de desarrollo

## Limitaciones

- El tiempo de CPU, la RAM y el swap son simulados.
- Los PID de MiniSO no son los PID de Linux.
- No se modifica el sistema de archivos del usuario ni se lanza una fork bomb.

## Fase 2

No está implementada. Continuar desde `docs/DEVELOPMENT_STATE.md` cuando se autorice Qt.
