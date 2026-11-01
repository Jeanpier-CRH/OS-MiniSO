# Requisitos Fase 1

- C++17, CMake, Linux, sin Qt en el núcleo.
- Procesos simulados con BCP y estados NEW/READY/RUNNING/BLOCKED/TERMINATED.
- PID de usuario desde 100; PID 1–6 reservados.
- Round Robin configurable.
- First-Fit y paginación independientes y usables desde la consola.
- Swap FIFO y fallos de página simulados.
- Productor-consumidor con pthreads reales.
- Process runner con fork/exec/waitpid reales.
- Escenarios A–D y MiniStress con OOM simulado.
- Eventos consultables y exportables.
- Pruebas automatizadas con CTest.
