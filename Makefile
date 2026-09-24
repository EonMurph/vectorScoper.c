src = main.c
exe = vectorscoper

$(exe): $(src)
	clang main.c -I/usr/local/include -I/opt/homebrew/opt/libpng/include -I/opt/homebrew/opt/jpeg/include -L/opt/homebrew/opt/libpng/lib -L/opt/homebrew/opt/jpeg/lib -L/usr/local/lib /usr/local/lib/libimago.a -ljpeg -lpng -lz -o $(exe)

clean:
	rm $(exe)
