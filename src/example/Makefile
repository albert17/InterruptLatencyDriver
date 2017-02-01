# export the environment variable LINARO properly set

BIN = main 
CC = $(LINARO)/bin/arm-linux-gnueabihf-gcc
INC = -I$(LINARO)/arm-linux-gnueabihf/libc/usr/include
LIBS = -static -L$(LINARO)/arm-linux-gnueabihf/libc/lib/arm-linux-gnueabihf -L$(LINARO)/arm-linux-gnueabihf/libc/usr/lib/arm-linux-gnueabihf

main: main.c
		$(CC) $(CFLAGS) $(INC) $(LIBS) -o $(BIN) main.c 

clean:
		rm $(BIN)
