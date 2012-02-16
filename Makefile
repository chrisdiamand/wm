

OBJ=alttab.o event.o policy.o wm.o client.o
OUT=wm
LIBS=-lX11
CFLAGS=-g -Wall -pedantic

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) -o $(OUT) $(OBJ) $(LIBS)

clean:
	rm -f $(OUT) $(OBJ)
