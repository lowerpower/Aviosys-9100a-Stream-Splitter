
TARGET	=	splitter

all: $(TARGET)

# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = 

#deines for build env
CDEFINE = -DLINUX 

# Options for development
CFLAGS		= $(INCLUDE) $(CDEFINE) -g -Wall
LDFLAGS		=
LINKFLAGS	= -o 
LIBS		=

# Options for release
# CFLAGS = -O -Wall -ansi

#Dependancies for all
DEPENDALL	= config.h mytypes.h

#Objects
OBJ			= splitter.o 

$(TARGET): $(OBJ) 
	$(CC) $(LINKFLAGS) $(TARGET) $(OBJ) $(LIBS)

clean:
	-rm *.o $(MYLIB)



