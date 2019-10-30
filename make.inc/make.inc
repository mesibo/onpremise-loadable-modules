CC = gcc
FLAGS        = # -std=gnu99 -Iinclude
CFLAGS       = -I../include -DMESIBO_MODULE=$(MODULE) -fPIC -g #-pedantic -Wall -Wextra -ggdb3
LDFLAGS      = -shared 
RM = rm -f

ifndef OBJPATH
OBJPATH=../bin/$(MODULE)
endif

SRC    = $(wildcard *.c)
OBJ := $(patsubst %.c, $(OBJPATH)/%.o, $(SRC))


DEBUGFLAGS   = -O0 -D _DEBUG
RELEASEFLAGS = -O2 -D NDEBUG -combine -fwhole-program

TARGET  = /usr/lib64/mesibo/mesibo_mod_$(MODULE).so

all: $(TARGET)

clean: 
	$(RM) $(OBJ) $(TARGET)

$(OBJPATH)/%.o: %.c $(MAKEFILEDEP)
	@mkdir -p $(OBJPATH)
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJECTS): ../include/module.h

$(TARGET): $(OBJ) ../include/module.h
	@mkdir -p /usr/lib64/mesibo/
	$(CC) $(FLAGS) $(LDFLAGS) $(DEBUGFLAGS) -o $(TARGET) $(OBJ)

