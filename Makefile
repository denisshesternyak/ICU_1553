CC := gcc
CXX := g++
CFLAGS := -fPIC -Wall -g
CPPFLAGS := -fPIC -Wall -g
LDFLAGS := -L ./lib -fPIC
LIBS := -lExc1553Px -lExc4000 -lexchost2dpr -ldl -lrt -lpthread -lftd2 -lftd3
PROGS += platform icu irst

BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin

PLATFORM_SRC_DIR := src/platform
ICU_SRCS_DIR := src/icu
IRST_SRCS_DIR := src/irst

PLATFORM_SRC := $(PLATFORM_SRC_DIR)/platform.c
ICU_SRCS := $(ICU_SRCS_DIR)/icu.c $(ICU_SRCS_DIR)/config.c $(ICU_SRCS_DIR)/mil_std_1553.c $(ICU_SRCS_DIR)/client.c $(ICU_SRCS_DIR)/ini.c
IRST_SRC := $(IRST_SRCS_DIR)/irst.c

PLATFORM_OBJ := $(BUILD_DIR)/platform.o
ICU_OBJS := $(patsubst $(ICU_SRCS_DIR)/%.c, $(BUILD_DIR)/%.o, $(ICU_SRCS))
IRST_OBJ := $(BUILD_DIR)/irst.o

all: $(BIN_DIR)/platform $(BIN_DIR)/icu $(BIN_DIR)/irst

$(BUILD_DIR)/%.o: $(PLATFORM_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. -I./lib -c $< -o $@

$(BUILD_DIR)/%.o: $(ICU_SRCS_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. -I./lib -c $< -o $@

$(BUILD_DIR)/%.o: $(IRST_SRCS_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. -I./lib -c $< -o $@

$(BIN_DIR)/platform: $(PLATFORM_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

$(BIN_DIR)/icu: $(ICU_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

$(BIN_DIR)/irst: $(IRST_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	rm -rf $(BUILD_DIR)
