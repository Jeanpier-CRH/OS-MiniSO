# Plan de pruebas Fase 1

| ID | Objetivo | Entrada | Esperado | Cómo se ejecuta |
|---|---|---|---|---|
| P-01 | Procesos de sistema | boot | PID 1–6 vivos | test_processes |
| P-02 | PID/PPID | dos procesos | 100, 101, PPID 1 | test_processes |
| P-03 | Árbol y cascada | MiniBrowser + tabs | hijos terminados | test_processes |
| S-01 | Un proceso | burst 4, Q=3 | TERMINATED, cpu=4 | test_scheduler |
| S-02 | Dos procesos | bursts 5, Q=2 | ambos con CPU > 0 | test_scheduler |
| M-01 | First-Fit | 100 bytes | split y coalesce | test_partition |
| M-02 | Fragmentación externa | huecos no contiguos | allocate grande falla | test_partition |
| G-01 | Traducción | dir page+128 | physical correcto | test_paging |
| G-02 | Fragmentación interna | 10 MB / 4 MB | 2 MB | test_paging |
| W-01 | FIFO | RAM 2 marcos | fault y recover | test_swap |
| O-01 | MiniStress | 40 procesos 16 MB | OOM o menos de 40 | test_oom |
| C-01 | 2P2C | 40 items | produced==consumed | test_concurrency |
| I-01 | App open/close | MiniEditor | memoria y log | test_integration |
| I-02 | Linux demo | process_runner | ≥2 hijos | test_integration |
| CFG-01 | Configuración JSON | config/default_config.json | carga válida y rangos correctos | test_config |
| CFG-02 | Configuración inválida | RAM cero | carga rechazada | test_config |

## Resultados obtenidos

Ejecutados el 2026-11-01 con:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

Resultado: 9/9 suites aprobadas, 0 fallos, 0.06 segundos.

Evidencia adicional: [phase1/ctest.txt](evidence/phase1/ctest.txt).
