

OBJ=alttab.o client.o event.o launcher.o policy.o rc.o wm.o
OUT=wm
LIBS=`pkg-config --libs xproto` -lX11
CFLAGS=`pkg-config --cflags xproto` -g -Wall -pedantic

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) -o $(OUT) $(OBJ) $(LIBS)

clean:
	rm -f $(OUT) $(OBJ)
