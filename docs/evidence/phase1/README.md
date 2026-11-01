# Evidencias de Fase 1

La carpeta contiene registros generados por la versión validada del proyecto.

- `demo.txt`: salida de `./build/miniso --demo`.
- `linux.txt`: salida de `./build/process_runner --demo --child ./build/process_child`.
- `ctest.txt`: resultado completo de las nueve suites CTest.

Generación reproducible:

```bash
./build/miniso --demo > docs/evidence/phase1/demo.txt
./build/process_runner --demo --child ./build/process_child > docs/evidence/phase1/linux.txt
ctest --test-dir build --output-on-failure > docs/evidence/phase1/ctest.txt 2>&1
```

Resultado validado: 9/9 suites aprobadas. Las evidencias incluyen procesos Linux reales, procesos simulados, paginación, productor-consumidor, integración y carga de configuración.
