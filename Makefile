all: main

main: main.c ; clang -framework CoreFoundation -framework IOKit main.c -o unfucker

