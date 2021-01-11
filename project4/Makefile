
CC = g++

CPP_FLAGS = -O3

LINKFLAGS_GPU = -O3

COMPILEFLAGS = -O3 -std=c++11 -Wall -I include/

##########
# OBJECTS
##########

BIN_DIR = bin

OBJ_DIR = bin

OBJS = $(OBJ_DIR)/cliente.o $(OBJ_DIR)/servidor.o

#################################################################################################################################

all: create_dir $(OBJS)

create_dir:
		mkdir -p $(BIN_DIR) $(OBJ_DIR)

$(OBJS): $(OBJ_DIR)/%.o: src/%.cpp
		$(CC) $< -o $@ $(COMPILEFLAGS)

#################################################################################################################################

clean:
		rm -rf $(OBJ_DIR) $(BIN_DIR)
		