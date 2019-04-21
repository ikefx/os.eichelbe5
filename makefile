# Neil Eichelberger
# cs4760 Assignment 05 makefile
# 
#

CC = cc -lpthread -pthread
RM = rm
CFLAGS = -Wall -ansi -lm -std=gnu99
LIBS = -lrt -lpthread
.SUFFIXES:
.SUFFIXES: .c .o .h

all: oss user

# explicit
oss: oss.o
	$(CC) $(CFLAGS) -o oss oss.o $(LIBS)
user: user.o
	$(CC) $(CFLAGS) -o user user.o $(LIBS)
# implicit

oss.o: oss.c
	$(CC) -c $(CFLAGS) oss.c
user.o: user.c 
	$(CC) -c $(CFLAGS) user.c

# clean up
clean:
	$(RM) -f $(OBJ) oss.o oss user.o user
