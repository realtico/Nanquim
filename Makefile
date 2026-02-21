CC = gcc
# Inclui -fPIC para permitir bibliotecas compartilhadas
CFLAGS = -std=c11 -Wall -Wextra -O2 -fPIC -Iinclude $(shell pkg-config --cflags sdl2)
# Libs necessarias (Link final)
LDFLAGS = $(shell pkg-config --libs sdl2) -lSDL2_gfx -lm
# Flag especifica para criar .so
LIB_LDFLAGS = -shared

SRC_DIR = src
OBJ_DIR = build
BIN_DIR = bin
LIB_DIR = lib

# Core Sources for the Library
CORE_SRCS = $(SRC_DIR)/nq_core.c $(SRC_DIR)/nq_draw.c $(SRC_DIR)/nq_assets.c $(SRC_DIR)/nq_input.c
# Troca .c por .o no dir de objectos
CORE_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CORE_SRCS))

# Shared Library Target
LIB_NAME = libnanquim.so
LIB_TARGET = $(LIB_DIR)/$(LIB_NAME)

# Example Executables
TEST_PATTERN = $(BIN_DIR)/test_pattern
STATIC_TEST = $(BIN_DIR)/static_test
DIFFRACTION_SIM = $(BIN_DIR)/diffraction_sim
INPUT_DEMO = $(BIN_DIR)/input_demo

ALL_EXAMPLES = $(TEST_PATTERN) $(STATIC_TEST) $(DIFFRACTION_SIM) $(INPUT_DEMO)


.PHONY: all clean directories

all: directories $(LIB_TARGET) $(ALL_EXAMPLES)

# 1. Build Shared Library
$(LIB_TARGET): $(CORE_OBJS)
	$(CC) $(LIB_LDFLAGS) -o $@ $^ $(LDFLAGS)

# 2. Compile Core Objects (generic rule)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 3. Build Examples (Linking against libnanquim.so)
# Note: -Wl,-rpath,$(PWD)/$(LIB_DIR) permite rodar sem instalar a so no /usr/lib

$(TEST_PATTERN): examples/test_pattern.c $(LIB_TARGET)
	$(CC) $(CFLAGS) $< -o $@ -L$(LIB_DIR) -lnanquim $(LDFLAGS) -Wl,-rpath,$(PWD)/$(LIB_DIR)

$(STATIC_TEST): examples/static_test.c $(LIB_TARGET)
	$(CC) $(CFLAGS) $< -o $@ -L$(LIB_DIR) -lnanquim $(LDFLAGS) -Wl,-rpath,$(PWD)/$(LIB_DIR)

$(DIFFRACTION_SIM): examples/diffraction_sim.c $(LIB_TARGET)
	$(CC) $(CFLAGS) $< -o $@ -L$(LIB_DIR) -lnanquim $(LDFLAGS) -Wl,-rpath,$(PWD)/$(LIB_DIR)

$(INPUT_DEMO): examples/input_demo.c $(LIB_TARGET)
	$(CC) $(CFLAGS) $< -o $@ -L$(LIB_DIR) -lnanquim $(LDFLAGS) -Wl,-rpath,$(PWD)/$(LIB_DIR)

directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)
