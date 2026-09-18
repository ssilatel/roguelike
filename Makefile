build:
	gcc main.c -lSDL2 -lSDL2_image -lm

dev:
	gcc -Wall -Werror -pedantic main.c -lSDL2 -lSDL2_image

run:
	./a.out

windows:
	x86_64-w64-mingw32-gcc main.c -o game.exe -I/usr/x86_64-w64-mingw32/include/SDL2 -L/usr/x86_64-w64-mingw32/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -Wl,--subsystem,windows
