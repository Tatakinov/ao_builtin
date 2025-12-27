CXXFLAGS=-g -O2 -DUSE_WAYLAND -Wall -std=c++20 -I . -I include -I libfontlist/include $(shell pkg-config --cflags fontconfig sdl3 sdl3-ttf stb wayland-client)
LDFLAGS=-L . $(shell pkg-config --libs fontconfig sdl3 sdl3-ttf stb wayland-client)
OBJ=$(shell find -name "*.cc" | sed -e 's/\.cc$$/.o/g') $(shell find libfontlist/src -name "*.cpp" | sed -e 's/\.cpp$$/.o/g')
TARGET=_builtin.exe

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(TARGET) $(OBJ)
