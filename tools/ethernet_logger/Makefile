# Makefile with dependency tracking and multi-platform support

# Default Config
EXE_NAME = eth_logger
BUILD_DIR = build/mac
CXX = c++
CXXFLAGS = -std=c++11 -Ivendor/imgui -Ivendor/imgui/backends -I. -g -Wall -Wformat
LIBS = 

# Detect OS for default run
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Linux)
	ECHO_MESSAGE = "Linux"
	LIBS += -lGL -ldl `sdl2-config --libs`
	CXXFLAGS += `sdl2-config --cflags`
endif
ifeq ($(UNAME_S), Darwin)
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo `sdl2-config --libs`
	LIBS += -L/usr/local/lib -L/opt/homebrew/lib
	CXXFLAGS += -I/usr/local/include -I/opt/homebrew/include -I/usr/local/include/SDL2 -I/opt/homebrew/include/SDL2 -D_THREAD_SAFE
endif

# Source Files
VPATH = src/core:src/cli:src/gui:vendor/imgui:vendor/imgui/backends
SOURCES = main.cpp \
          Filter.cpp Device.cpp Capture.cpp \
          CliApp.cpp GuiApp.cpp \
          vendor/imgui/imgui.cpp vendor/imgui/imgui_demo.cpp vendor/imgui/imgui_draw.cpp \
          vendor/imgui/imgui_tables.cpp vendor/imgui/imgui_widgets.cpp \
          vendor/imgui/backends/imgui_impl_sdl2.cpp vendor/imgui/backends/imgui_impl_opengl3.cpp

# Object Files in Build Directory
OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))
DEPS = $(OBJS:.o=.d)

# Dependency Flags
DEPFLAGS = -MMD -MP

# Default Target
all: $(BUILD_DIR)/$(EXE_NAME)
	@echo Build complete for $(ECHO_MESSAGE)

# Link Rule
$(BUILD_DIR)/$(EXE_NAME): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $(OBJS) $(CXXFLAGS) $(LIBS)

# Compilation Rule
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c -o $@ $<

# Include Dependencies
-include $(DEPS)

# Windows Config (MinGW)
MINGW_CXX = x86_64-w64-mingw32-g++
MINGW_EXE = eth_logger.exe
MINGW_BUILD_DIR = build/win
MINGW_LIBS = -Lvendor/sdl2-win/lib -Lvendor/npcap-sdk/Lib/x64 -lmingw32 -lSDL2main -lSDL2 -lws2_32 -lwpcap -lpacket -lopengl32 -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -luuid -static-libgcc -static-libstdc++
MINGW_CXXFLAGS = -std=c++11 -Ivendor/imgui -Ivendor/imgui/backends -Ivendor/sdl2-win/include -Ivendor/sdl2-win/include/SDL2 -Ivendor/npcap-sdk/Include -D_WIN32 -DWIN32 -g -Wall -Wformat -I.

# Windows Target (Separate Build Dir)
windows:
	@$(MAKE) --no-print-directory \
		EXE_NAME=$(MINGW_EXE) \
		BUILD_DIR=$(MINGW_BUILD_DIR) \
		CXX=$(MINGW_CXX) \
		CXXFLAGS="$(MINGW_CXXFLAGS)" \
		LIBS="$(MINGW_LIBS)" \
		ECHO_MESSAGE="Windows (MinGW)"
	@cp vendor/sdl2-win/bin/SDL2.dll $(MINGW_BUILD_DIR)/ 2>/dev/null || true
	@cp $(MINGW_BUILD_DIR)/$(MINGW_EXE) .

clean:
	rm -rf build eth_logger eth_logger.exe

