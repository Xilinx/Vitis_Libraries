#!/usr/bin/bash
find . -type f -name "*.tcl" -exec sed -i 's/add_files src\/main.cpp/add_files src\/main.cpp -cflags "-I..\/..\/..\/..\/..\/..\/include\/hw\/xf_fft\/fixed\/"/g' {} +

