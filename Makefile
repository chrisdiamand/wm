
RC_OBJ=rc/parser.o rc/rc.o rc/scanner.o
OBJ=client.o event.o launcher.o policy.o switcher.o wm.o wmprefs.o $(RC_OBJ)
OUT=wm
LDFLAGS=`pkg-config --libs xproto` -lX11
CFLAGS=`pkg-config --cflags xproto` -g -Wall -pedantic
PREFIX=/usr


all: $(OUT)

$(OUT): $(OBJ)

install: $(OUT)
	echo "This is a new make install command. prefix = $(PREFIX)"
	install -d $(PREFIX)/bin/
	install -m 755 $(OUT) $(PREFIX)/bin/

uninstall:
	rm -f $(PREFIX)/bin/$(OUT)

clean:
	rm -f $(OUT) $(OBJ)

tar:
	rm -f ../wm.tar.gz
	tar zcvf ../wm.tar.gz *
