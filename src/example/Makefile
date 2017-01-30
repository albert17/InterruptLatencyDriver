# export the environment variable LINARO properly set

BIN = main 
CC = $(LINARO)/bin/arm-linux-gnueabihf-gcc
INC = -I$(LINARO)/arm-linux-gnueabihf/libc/usr/include

main: main.c
		$(CC) $(CFLAGS) $(INC) -o $(BIN) main.c 

clean:
		rm $(BIN)
