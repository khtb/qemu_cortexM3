EXE = eth_logger
SOURCES = main.cpp 
SOURCES += vendor/imgui/imgui.cpp vendor/imgui/imgui_demo.cpp vendor/imgui/imgui_draw.cpp vendor/imgui/imgui_tables.cpp vendor/imgui/imgui_widgets.cpp
SOURCES += vendor/imgui/backends/imgui_impl_sdl2.cpp vendor/imgui/backends/imgui_impl_opengl3.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

CXXFLAGS = -std=c++11 -Ivendor/imgui -Ivendor/imgui/backends
CXXFLAGS += -g -Wall -Wformat

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += -lGL -ldl `sdl2-config --libs`
	CXXFLAGS += `sdl2-config --cflags`
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo `sdl2-config --libs`
	LIBS += -L/usr/local/lib -L/opt/homebrew/lib
	#CXXFLAGS += `sdl2-config --cflags`
	CXXFLAGS += -I/usr/local/include -I/opt/homebrew/include -I/usr/local/include/SDL2 -I/opt/homebrew/include/SDL2 -D_THREAD_SAFE
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:vendor/imgui/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:vendor/imgui/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS) eth_logger.exe

##---------------------------------------------------------------------
## WINDOWS BUILD (MINGW)
##---------------------------------------------------------------------

# Verify if CXX is set to mingw, otherwise use a default
MINGW_CXX = x86_64-w64-mingw32-g++
MINGW_EXE = eth_logger.exe
# Flags for Windows
# We need to link against ws2_32 for Winsock, and SDL2/OpenGL
MINGW_LIBS = -Lvendor/sdl2-win/lib -lmingw32 -lSDL2main -lSDL2 -lws2_32 -lopengl32 -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -luuid -static-libgcc -static-libstdc++
MINGW_CXXFLAGS = -std=c++11 -Ivendor/imgui -Ivendor/imgui/backends -Ivendor/sdl2-win/include -Ivendor/sdl2-win/include/SDL2 -D_WIN32 -g -Wall -Wformat

windows: 
	$(MAKE) EXE=$(MINGW_EXE) CXX=$(MINGW_CXX) CXXFLAGS="$(MINGW_CXXFLAGS)" LIBS="$(MINGW_LIBS)" clean all
	cp vendor/sdl2-win/bin/SDL2.dll .
