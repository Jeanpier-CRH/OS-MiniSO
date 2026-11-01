# Manual de usuario — consola

Arranque:

```bash
./build/miniso
```

Menú principal:

1. Process Manager — listar, crear, consultar BCP, árbol, terminar.
2. CPU Scheduler — tick, 20 ticks, quantum, snapshot.
3. Memory Manager — particiones, marcos, tablas, traducción, swap.
4. Thread Manager — productor-consumidor y MiniRenderer.
5. Desktop Applications — abrir las cinco apps y pestañas del navegador.
6. Scenario Laboratory — A ciclo de vida, B page fault, C OOM, D hilos.
7. Linux Real Processes — ejecuta `process_runner`.
8. System Event Log — últimos eventos.
9. System Status — resumen y árbol.
0. Shutdown — cierra hilos y procesos lógicos.

Los PID que muestra el administrador de tareas son simulados. Los PID del apartado 7 son PID reales de Linux.

Para contrastar procesos reales:

```bash
./build/process_runner --demo --child ./build/process_child
ps -o pid,ppid,cmd
```
