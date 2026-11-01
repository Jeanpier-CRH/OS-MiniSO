Generar evidencias de Fase 1:

```bash
./build/miniso --demo > docs/evidence/phase1/demo.txt
./build/process_runner --demo --child ./build/process_child > docs/evidence/phase1/linux.txt
ctest --test-dir build --output-on-failure | tee docs/evidence/phase1/ctest.txt
```
