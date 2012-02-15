

OBJ=alttab.o policy.o wm.o client.o
OUT=wm
LIBS=-lX11
CFLAGS=-Wall -pedantic

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) -o $(OUT) $(OBJ) $(LIBS)

clean:
	rm -f $(OUT) $(OBJ)
