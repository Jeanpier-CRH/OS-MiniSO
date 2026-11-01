# MiniSO Lab - Development State

## Información general

**Proyecto:** MiniSO Lab  
**Estado general:** En desarrollo  
**Fase actual:** Phase 1 - Core and Console Development

---

# Estado actual del proyecto

## Fase 1 - Configuración del repositorio

Estado: 🟡 En progreso

### Completado

- [x] Creación del repositorio GitHub.
- [x] Configuración inicial de `.gitignore`.
- [x] Configuración de GitHub Actions.
- [x] Creación de plantilla para Pull Requests.
- [x] Definición inicial del flujo de trabajo con Issues y Pull Requests.
- [x] Configuración inicial de CMake.
- [x] Configurar protección de la rama `main` y `develop`.
- [x] Configurar GitHub Projects.

### Pendiente

- [ ] Crear Issues iniciales.
- [ ] Crear Milestones.

---

# Módulos del sistema

## 1. Gestión de procesos

**Estado:** 🔴 No iniciado

### Funcionalidades previstas

- [ ] Definir la estructura PCB / BCP.
- [ ] Implementar Process Manager.
- [ ] Crear procesos reales mediante `fork()`.
- [ ] Ejecutar programas mediante `exec()`.
- [ ] Obtener PID y PPID.
- [ ] Implementar relación padre-hijo.
- [ ] Implementar finalización mediante `wait()` o `waitpid()`.
- [ ] Registrar estados de procesos.
- [ ] Implementar procesos simulados del sistema.
- [ ] Implementar monitor de procesos.

---

## 2. Planificación de CPU

**Estado:** 🔴 No iniciado

### Funcionalidades previstas

- [ ] Implementar algoritmo Round Robin.
- [ ] Implementar cola de procesos Ready.
- [ ] Implementar quantum configurable.
- [ ] Simular cambios de contexto.
- [ ] Registrar transiciones de estado.
- [ ] Mostrar proceso actualmente en ejecución.

---

## 3. Hilos y concurrencia

**Estado:** 🔴 No iniciado

### Funcionalidades previstas

- [ ] Implementar POSIX Threads.
- [ ] Implementar Productor-Consumidor.
- [ ] Implementar al menos dos productores.
- [ ] Implementar al menos dos consumidores.
- [ ] Implementar buffer limitado.
- [ ] Implementar exclusión mutua mediante mutex.
- [ ] Implementar variables de condición.
- [ ] Mostrar operaciones concurrentes.
- [ ] Garantizar ausencia de pérdidas o duplicaciones.
- [ ] Documentar escenario hipotético de deadlock.

---

## 4. Gestión de memoria

**Estado:** 🔴 No iniciado

### Particionamiento

- [ ] Definir memoria total configurable.
- [ ] Implementar First-Fit.
- [ ] Permitir asignación de memoria.
- [ ] Permitir liberación de memoria.
- [ ] Detectar fragmentación externa.
- [ ] Unir bloques libres contiguos.

### Paginación

- [ ] Definir tamaño de página.
- [ ] Definir cantidad de marcos.
- [ ] Implementar tabla de páginas.
- [ ] Implementar traducción lógica-física.
- [ ] Detectar direcciones inválidas.
- [ ] Implementar Page Fault simulado.
- [ ] Implementar algoritmo FIFO.
- [ ] Simular memoria virtual / swap básico.

---

# Aplicaciones simuladas

**Estado:** 🔴 No iniciado

Aplicaciones previstas:

- [ ] Navegador Web.
- [ ] Editor de Texto.
- [ ] Monitor de Sistema.
- [ ] Renderizador.
- [ ] Aplicación de prueba de memoria.

---

# Integración entre módulos

**Estado:** 🔴 No iniciado

Funcionalidades previstas:

- [ ] Los procesos tendrán información de memoria simulada.
- [ ] Los procesos podrán crear procesos hijos.
- [ ] El planificador gestionará procesos simulados.
- [ ] Los procesos podrán utilizar hilos reales.
- [ ] La finalización de procesos liberará memoria simulada.
- [ ] Los eventos de memoria podrán afectar a los hilos simulados.

---

# Fase 2 - Interfaz gráfica con Qt

**Estado:** ⚪ No iniciada

### Funcionalidades previstas

- [ ] Integrar Qt Creator.
- [ ] Crear Desktop de MiniSO Lab.
- [ ] Crear Process Dashboard.
- [ ] Crear Memory Dashboard.
- [ ] Crear Thread Monitor.
- [ ] Crear ventanas de aplicaciones.
- [ ] Conectar interfaz gráfica con MiniSO Core.
- [ ] Mantener el núcleo independiente de Qt.

---

# Última compilación

**Fecha:** Pendiente

## Comando

```bash
cmake -S . -B build
cmake --build build
```

## Resultado

```text
Pendiente de implementación
```

---

# Pruebas

**Estado:** 🔴 No iniciadas

Pendiente:

- [ ] Configurar framework de pruebas.
- [ ] Crear pruebas unitarias.
- [ ] Crear pruebas de integración.
- [ ] Configurar ejecución automática en GitHub Actions.

---

# Problemas conocidos

Actualmente no existen problemas funcionales registrados.

## Problemas de infraestructura

- El proyecto se encuentra en fase inicial.
- La estructura completa del código fuente todavía está pendiente de implementación.

---

# Próximas tareas prioritarias

1. Definir la arquitectura definitiva del Core.
2. Crear la estructura inicial de `src/`, `include/` y `tests/`.
3. Configurar el sistema de compilación CMake.
4. Implementar la estructura PCB / BCP.
5. Implementar la gestión básica de procesos.
6. Crear las primeras pruebas.
7. Actualizar GitHub Actions para una validación estricta cuando exista código funcional.

---

# Historial de desarrollo

## Etapa 1 - Configuración inicial

**Estado:** En progreso

Actividades realizadas:

- Creación del repositorio.
- Definición del flujo de trabajo con Git.
- Configuración inicial de GitHub Actions.
- Preparación de documentación del proyecto.

---

**Última actualización:** 2026-11-01