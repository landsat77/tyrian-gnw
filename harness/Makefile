# Tyrian (OpenTyrian) — headless Linux harness
#
#   make            → build/ty_host
#   make SAN=1      → with AddressSanitizer + UBSan
#
#   ./build/ty_host Tyrian.ty -t 20000 -e 1000 -o /tmp/ty

BUILD_DIR = build
TARGET = $(BUILD_DIR)/ty_host

OT = ../src/ot
PORT = ../src/port

include $(OT)/sources.mk

OT_SOURCES = $(addprefix $(OT)/,$(OT_FILES) $(OT_RENDER_FILES))
PORT_SOURCES = \
	$(PORT)/ty_alloc.c \
	$(PORT)/ty_pack.c \
	$(PORT)/ty_file.c \
	$(PORT)/ty_video.c \
	$(PORT)/ty_sdl.c \
	$(PORT)/ty_core.c \
	$(PORT)/ty_loudness.c \
	$(PORT)/ty_state.c \
	$(PORT)/ty_music.c \
	$(PORT)/ty_render.c \
	ty_host.c

SAN ?= 0
OPT ?= -O2 -g
ifeq ($(SAN),1)
OPT += -fsanitize=address,undefined -fno-omit-frame-pointer -DTY_HOST_SYSTEM_MALLOC
LDFLAGS += -fsanitize=address,undefined
endif

INCLUDES = -I$(PORT) -I$(PORT)/sdl -I$(OT)
WARN = -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function -Wno-missing-braces
GLUE = -include $(PORT)/gnw_ty_glue.h
CFLAGS = $(OPT) -MMD -std=gnu11 -DGNW -DNDEBUG $(WARN) $(INCLUDES)
# the game and the port code it calls use the game heap
GAME_CFLAGS = $(CFLAGS) $(GLUE)
GLUED = ty_file ty_video ty_sdl ty_core ty_loudness ty_state

OT_OBJECTS = $(addprefix $(BUILD_DIR)/ot_,$(OT_FILES:.c=.o) $(OT_RENDER_FILES:.c=.o))
PORT_OBJECTS = $(addprefix $(BUILD_DIR)/p_,$(notdir $(PORT_SOURCES:.c=.o)))

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/ot_%.o: $(OT)/%.c | $(BUILD_DIR)
	$(CC) -c $(GAME_CFLAGS) $< -o $@

$(addprefix $(BUILD_DIR)/p_,$(addsuffix .o,$(GLUED))): $(BUILD_DIR)/p_%.o: $(PORT)/%.c | $(BUILD_DIR)
	$(CC) -c $(GAME_CFLAGS) $< -o $@

$(BUILD_DIR)/p_%.o: $(PORT)/%.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/p_ty_host.o: ty_host.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(OT_OBJECTS) $(PORT_OBJECTS)
	$(CC) $^ $(LDFLAGS) -lm -o $@

-include $(wildcard $(BUILD_DIR)/*.d)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
