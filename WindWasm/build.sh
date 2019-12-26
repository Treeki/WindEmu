#!/bin/sh

FLAGS="-O3 -s WASM_OBJECT_FILES=0 -std=c++17"

mkdir -p obj
for i in arm710 emubase etna windermere; do emcc -c $FLAGS -o obj/$i.o ../WindCore/$i.cpp; done
emcc $FLAGS -s TOTAL_MEMORY=78643200 --llvm-lto 1 --preload-file rom/5mx.bin --shell-file shell.html obj/*.o main.cpp -o WindEmu.html