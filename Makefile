CC := gcc
CXX := g++
CFLAGS := -fPIC -Wall -g
CPPFLAGS := -fPIC -Wall -g
LDFLAGS := -L ./lib -fPIC
LIBS := -lExc1553Px -lExc4000 -lexchost2dpr -ldl -lrt -lpthread -lftd2 -lftd3 -lstdc++
PROGS += platform icu irst

BUILD_DIR := build
BUILD_PLATFORM_DIR := $(BUILD_DIR)/platform
BUILD_ICU_DIR := $(BUILD_DIR)/icu
BUILD_IRST_DIR := $(BUILD_DIR)/irst

BIN_DIR := $(BUILD_DIR)/bin

PLATFORM_SRC_DIR := src/platform
ICU_SRCS_DIR := src/icu
IRST_SRCS_DIR := src/irst

PLATFORM_OBJ := $(patsubst $(PLATFORM_SRC_DIR)/%.c, $(BUILD_PLATFORM_DIR)/%.o, $(PLATFORM_SRC_DIR)/platform.c $(PLATFORM_SRC_DIR)/config.c $(PLATFORM_SRC_DIR)/mil_std_1553.c $(PLATFORM_SRC_DIR)/ini.c)
ICU_OBJS := $(patsubst $(ICU_SRCS_DIR)/%.c, $(BUILD_ICU_DIR)/%.o, $(ICU_SRCS_DIR)/icu.c $(ICU_SRCS_DIR)/config.c $(ICU_SRCS_DIR)/mil_std_1553.c $(ICU_SRCS_DIR)/client.c $(ICU_SRCS_DIR)/ini.c)
IRST_OBJ := $(patsubst $(IRST_SRCS_DIR)/%.c, $(BUILD_IRST_DIR)/%.o, $(IRST_SRCS_DIR)/irst.c $(IRST_SRCS_DIR)/config.c $(IRST_SRCS_DIR)/ini.c $(IRST_SRCS_DIR)/server.c)


all: $(BIN_DIR)/platform $(BIN_DIR)/icu $(BIN_DIR)/irst


$(BUILD_PLATFORM_DIR)/%.o: $(PLATFORM_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. -I./lib -c $< -o $@

$(BUILD_ICU_DIR)/%.o: $(ICU_SRCS_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. -I./lib -c $< -o $@

$(BUILD_IRST_DIR)/%.o: $(IRST_SRCS_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. -I./lib -c $< -o $@

$(BIN_DIR)/platform: $(PLATFORM_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(BIN_DIR)/icu: $(ICU_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(BIN_DIR)/irst: $(IRST_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	rm -rf $(BUILD_DIR)
