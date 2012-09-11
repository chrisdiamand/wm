
LAUNCHER_OBJ = launcher/launcher.o launcher/read_desktop_file.o launcher/menuitems.o
RC_OBJ = rc/parser.o rc/rc.o rc/scanner.o
UTILS_OBJ = utils/list.o utils/selectbox.o
OBJ = client.o event.o policy.o switcher.o wm.o wmprefs.o \
	  $(LAUNCHER_OBJ) $(RC_OBJ) $(UTILS_OBJ)
OUT = wm
INCLUDES = -I. -Ilauncher -Irc -Iutils
LDFLAGS += `pkg-config --libs xproto` `pkg-config --libs xinerama` -lX11
CFLAGS += `pkg-config --cflags xproto` `pkg-config --cflags xinerama` $(INCLUDES) -g -Wall -pedantic
PREFIX = /usr


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
