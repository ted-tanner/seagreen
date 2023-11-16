clang -O0 -DCGN_DEBUG -g -std=c11 testmain.c src/*.c src/cgninternals/*.c src/cgninternals/*.S -Iinclude && ./a.out
