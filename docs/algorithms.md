# Algoritmos

## Round Robin

1. Extraer el primero de la cola READY.
2. Pasarlo a RUNNING y asignar quantum `Q`.
3. Consumir 1 unidad de CPU por tick.
4. Si `remainingCpuTime == 0`, terminar.
5. Si el quantum se agota, volver a READY al final de la cola.
6. Complejidad por tick: O(1) amortizado sobre la cola.

Quantum por defecto: 3.

## First-Fit

Recorre los bloques desde la dirección 0. El primer hueco con `size >= request` se divide si sobra espacio. Al liberar, se fusionan huecos adyacentes. Complejidad O(n) sobre el número de bloques.

## Paginación y traducción

```
page   = logical / pageSize
offset = logical % pageSize
physical = frame * pageSize + offset
```

Ejemplo con página de 4 MB y proceso de 10 MB: 3 páginas, 2 MB de fragmentación interna.

## FIFO de swap

La cola FIFO guarda el orden de carga de marcos. La víctima es el marco más antiguo con `pinCount == 0`. La página pasa a swap, `present = false`.

## Productor-consumidor

Búfer circular. Espera en bucle sobre `pthread_cond_wait` para despertares espurios. Cierre: `stopRequested` + broadcast + `pthread_join`.

## OOM simulado

Puntuación: `weightMemory * logicalMemory - weightPriority * priority`. Los procesos de sistema están protegidos. No se mata ningún proceso del anfitrión.
