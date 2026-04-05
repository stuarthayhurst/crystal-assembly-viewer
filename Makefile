SHELL = bash -O globstar
BUILD_DIR ?= build

OBJECTS_SOURCE = $(shell ls ./src/**/*.c)
HEADERS_SOURCE = $(shell ls ./src/**/*.h)

OBJECT_DIR = $(BUILD_DIR)/objects
OBJECTS = $(subst ./src,$(OBJECT_DIR),$(subst .c,.o,$(OBJECTS_SOURCE)))

#Global arguments
CFLAGS += -Wall -Wextra -Werror -Wpedantic -flto=auto -std=c23

#Debug arguments
ifeq ($(DEBUG),true)
  CFLAGS += -g -fno-omit-frame-pointer
  CFLAGS += -fsanitize=address,undefined
endif

#Dependency arguments
CFLAGS += $(shell pkg-config --cflags libadwaita-1)
LDFLAGS += $(shell pkg-config --libs libadwaita-1)

$(BUILD_DIR)/crystal: $(OBJECTS)
	@mkdir -p "$(BUILD_DIR)"
	$(CC) -o "$(BUILD_DIR)/crystal" $(OBJECTS) $(CFLAGS) $(LDFLAGS)

$(OBJECT_DIR)/%.o: ./src/%.c $(HEADERS_SOURCE)
	@mkdir -p "$(OBJECT_DIR)"
	$(CC) "$<" -c $(CFLAGS) -o "$@"

.PHONY: build debug clean

build: $(BUILD_DIR)/crystal

debug:
	@DEBUG="true" $(MAKE) --no-print-directory build

clean:
	@rm -rfv "$(BUILD_DIR)"
