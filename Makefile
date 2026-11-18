# YSEdit GNU Make build
# Override tools/flags on the command line, e.g. make CXX=clang++.

CXX ?= c++
BUILD_DIR ?= build
TARGET := $(BUILD_DIR)/ysedit
SOURCES := src/main.cpp
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS ?=
LDFLAGS ?=

.PHONY: all release debug run clean install help

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) -o $@

release: CXXFLAGS := -std=c++17 -O2 -DNDEBUG -Wall -Wextra -Wpedantic
release: clean all

debug: CXXFLAGS := -std=c++17 -O0 -g3 -Wall -Wextra -Wpedantic
debug: clean all

run: all
	@test -n "$(FILE)" || (echo "Usage: make run FILE=main.shay"; exit 2)
	./$(TARGET) "$(FILE)"

clean:
	rm -rf $(BUILD_DIR)

PREFIX ?= /usr/local
install: all
	install -d "$(DESTDIR)$(PREFIX)/bin"
	install -m 755 "$(TARGET)" "$(DESTDIR)$(PREFIX)/bin/ysedit"

help:
	@echo "YSEdit build targets:"
	@echo "  make             Build YSEdit"
	@echo "  make debug       Build with debug symbols"
	@echo "  make release     Build optimized release binary"
	@echo "  make run FILE=…  Build and open a file"
	@echo "  make clean       Remove build output"
	@echo "  make install     Install to PREFIX (default /usr/local)"
