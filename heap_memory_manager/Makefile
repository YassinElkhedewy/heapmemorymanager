CC := gcc
AR := ar
ARFLAGS := rcs

SRC_DIR := src
INC_DIR := inc
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
DEP_DIR := $(BUILD_DIR)/dep
LIB_DIR := $(BUILD_DIR)/lib
TEST_DIR := test

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS := $(SRCS:$(SRC_DIR)/%.c=$(DEP_DIR)/%.d)
LIB := $(LIB_DIR)/libhmm.a
SHARED_LIB := $(LIB_DIR)/libhmm.so
TEST_APP := $(BUILD_DIR)/hmm_test

CPPFLAGS := -I$(INC_DIR)
CFLAGS := -Wall -Wextra -g -fPIC

.PHONY: all static_lib shared_lib test clean

all: static_lib test

static_lib: $(LIB)

$(LIB): $(OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^

shared_lib: $(SHARED_LIB)

$(SHARED_LIB): $(OBJS) | $(LIB_DIR)
	$(CC) -shared -o $@ $^

test: $(TEST_APP)

$(TEST_APP): $(OBJS) $(wildcard $(TEST_DIR)/*.c) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(wildcard $(TEST_DIR)/*.c) $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) $(DEP_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c $< -o $@

$(BUILD_DIR) $(OBJ_DIR) $(DEP_DIR) $(LIB_DIR):
	mkdir -p $@

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)