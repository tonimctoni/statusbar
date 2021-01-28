all:
	gcc -pedantic -Wall -O2 statusbar.c -lX11 -o statusbar
run: all
	./statusbar
install: all
	cp statusbar /root/my-applications/bin/
