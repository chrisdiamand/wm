

OBJ=alttab.o client.o event.o launcher.o policy.o wm.o
OUT=wm
LIBS=-lX11
CFLAGS=-g -Wall -pedantic

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) -o $(OUT) $(OBJ) $(LIBS)

clean:
	rm -f $(OUT) $(OBJ)
