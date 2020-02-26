CC = g++
FLAGS        = # -std=gnu99 -Iinclude
CFLAGS       = -I../include $(EXTRA_CCFLAGS) -DMESIBO_MODULE=$(MODULE) -fPIC -g -Wall #-pedantic -Wextra -ggdb3
LDFLAGS      = -shared $(EXTRA_LDFLAGS)
RM = rm -f

ifndef OBJPATH
OBJPATH=../bin/$(MODULE)
endif

SRC    = $(wildcard *.cpp)
OBJ := $(patsubst %.cpp, $(OBJPATH)/%.o, $(SRC))


DEBUGFLAGS   = -O0 -D _DEBUG
RELEASEFLAGS = -O2 -D NDEBUG -combine -fwhole-program

TARGET  = /usr/lib64/mesibo/mesibo_mod_$(MODULE).so

all: $(TARGET)

clean: 
	$(RM) $(OBJ) $(TARGET)

$(OBJPATH)/%.o: %.cpp $(MAKEFILEDEP)
	@mkdir -p $(OBJPATH)
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJECTS): ../include/module.h

$(TARGET): $(OBJ) ../include/module.h Makefile
	@mkdir -p /usr/lib64/mesibo/
	$(CC) $(FLAGS) $(LDFLAGS) $(DEBUGFLAGS) -o $(TARGET) $(OBJ)

