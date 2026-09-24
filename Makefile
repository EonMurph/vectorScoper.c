src = main.c

exe: $(src)
	clang main.c -I/usr/local/include -I/opt/homebrew/opt/libpng/include -L/opt/homebrew/opt/libpng/lib -L/usr/local/lib /usr/local/lib/libimago.a -ljpeg -lpng -lz -o exe

clean:
	rm exe
