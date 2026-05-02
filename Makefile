SHELL = bash -O globstar
BUILD_DIR ?= build

PREFIX_DIR ?= /usr/local
BINARY_DIR := $(PREFIX_DIR)/bin
SHARE_DIR := $(PREFIX_DIR)/share
DATA_DIR := $(SHARE_DIR)/io.github.stuarthayhurst.Crystal
ICON_DIR := $(SHARE_DIR)/icons/hicolor/scalable/apps
APPS_DIR := $(SHARE_DIR)/applications

OBJECTS_SOURCE = $(shell ls ./src/**/*.c)
HEADERS_SOURCE = $(shell ls ./src/**/*.h)

OBJECT_DIR = $(BUILD_DIR)/objects
OBJECTS = $(subst ./src,$(OBJECT_DIR),$(subst .c,.o,$(OBJECTS_SOURCE)))

#Global arguments
CFLAGS += -Wall -Wextra -Werror -Wpedantic -flto=auto -std=gnu23

#Debug arguments
ifeq ($(DEBUG),true)
  CFLAGS += -g -fno-omit-frame-pointer
  CFLAGS += -fsanitize=address,undefined
endif

#Dependency arguments
CFLAGS += $(shell pkg-config --cflags libadwaita-1 gtksourceview-5)
LDFLAGS += $(shell pkg-config --libs libadwaita-1 gtksourceview-5)

build: $(BUILD_DIR)/crystal $(BUILD_DIR)/asm-compiler.lang $(BUILD_DIR)/io.github.stuarthayhurst.Crystal.desktop

debug:
	@DEBUG="true" $(MAKE) --no-print-directory build

install: build
	install --strip -D -t "$(BINARY_DIR)" "$(BUILD_DIR)/crystal"
	install -m 0664 -D -t "$(DATA_DIR)" "$(BUILD_DIR)/asm-compiler.lang"
	install -m 0664 -D -t "$(APPS_DIR)" "$(BUILD_DIR)/io.github.stuarthayhurst.Crystal.desktop"
	install -m 0664 -D -t "$(ICON_DIR)" "data/io.github.stuarthayhurst.Crystal.svg"

uninstall:
	@rm -rv "$(BINARY_DIR)/crystal"
	@rm -rv "$(DATA_DIR)/asm-compiler.lang"
	@rm -rv "$(APPS_DIR)/io.github.stuarthayhurst.Crystal.desktop"
	@rm -rv "$(ICON_DIR)/io.github.stuarthayhurst.Crystal.svg"
	@rm -rfvi "$(DATA_DIR)"

clean:
	@rm -rfv "$(BUILD_DIR)"

.PHONY: build debug install uninstall clean

$(BUILD_DIR)/crystal: $(OBJECTS)
	@mkdir -p "$(BUILD_DIR)"
	$(CC) -o "$(BUILD_DIR)/crystal" $(OBJECTS) $(CFLAGS) $(LDFLAGS)

$(BUILD_DIR)/io.github.stuarthayhurst.Crystal.desktop: data/io.github.stuarthayhurst.Crystal.desktop
	@mkdir -p "$(BUILD_DIR)"
	@cp -v "$<" "$@"
	@echo "Path=$(BINARY_DIR)" >> "$@"

$(BUILD_DIR)/asm-compiler.lang: data/asm-compiler-base.lang data/asm-header.xml scripts/definitions.py scripts/generate-lang.py
	@mkdir -p "$(BUILD_DIR)"
	./scripts/generate-lang.py "data/asm-compiler-base.lang" "data/asm-header.xml" "$@"

$(OBJECT_DIR)/%.o: ./src/%.c $(HEADERS_SOURCE)
	@mkdir -p "$(OBJECT_DIR)"
	$(CC) "$<" -c $(CFLAGS) -o "$@"
