# -*- MakeFile -*- 

# Makefile settings
EXECUTABLE_NAME = GianSandBox.exe

# Create target name, build directory and executable path
BUILD_DIR = build
EXECUTABLE = $(BUILD_DIR)\$(EXECUTABLE_NAME)

LIB_SP_PATH=E:\Software_Libraries\LibSerialPort
STB_PATH=E:\Software_Libraries\Stb_Image

STB_SOURCES = \
$(STB_PATH)\Src\stb_image.c \
$(STB_PATH)\Src\stb_image_resize.c

LIB_SP_SOURCES = \
$(LIB_SP_PATH)\serialport.c \
$(LIB_SP_PATH)\timing.c \
$(LIB_SP_PATH)\windows.c

# The Windows/System32 include is required for the LibSerialPort setupapi.dll file
LIB_STB_INCLUDES = \
-I$(STB_PATH)

LIB_SP_INCLUDES = \
-I$(LIB_SP_PATH) \
-IC:/Windows/System32

LIB_SP_OBJECTS = $(patsubst %.c, %.o, $(LIB_SP_SOURCES))
STB_OBJECTS = $(patsubst %.c, %.o, $(STB_SOURCES))
LIB_SP_OBJECTS_BUILD_DIR = $(foreach D, $(LIB_SP_OBJECTS), $(BUILD_DIR)/$(notdir $(D)))
STB_OBJECTS_BUILD_DIR = $(foreach D, $(STB_OBJECTS), $(BUILD_DIR)/$(notdir $(D)))


GLEW_PATH=E:\Software_Libraries\OpenGL\glew-2.1.0
GLFW_PATH=E:\Software_Libraries\OpenGL\glfw-3.3.8

# Create list of .c/.cpp sources for each folder
CPP_LIB_SOURCES = $(shell dir /b /s "E:\Software_Libraries\Personal\Cpp\*.cpp")
C_LIB_SOURCES = $(shell dir /b /s "E:\Software_Libraries\Personal\C\*.c")
PROJECT_SOURCES_FULL = $(shell dir /b /s "*.cpp")

# Remove all .cpp files in the Discards folder
PROJECT_SOURCES = $(filter-out $(foreach S, $(PROJECT_SOURCES_FULL), $(if $(findstring Discards\Src\, $S), $S)), $(PROJECT_SOURCES_FULL))

GLEW_SOURCES = $(GLEW_PATH)\src\glew.c
GLFW_SOURCES = $(patsubst %.c, $(GLFW_PATH)\src\\%.c, monitor.c window.c win32_monitor.c win32_window.c init.c input.c context.c \
win32_init.c win32_thread.c win32_time.c win32_joystick.c wgl_context.c egl_context.c osmesa_context.c vulkan.c)

# Create list of .o files from .cpp and .c files
CPP_LIB_OBJECTS = $(patsubst %.cpp, $(BUILD_DIR)\\%.o, $(notdir $(CPP_LIB_SOURCES)))
C_LIB_OBJECTS = $(patsubst %.c, $(BUILD_DIR)\\%.o, $(notdir $(C_LIB_SOURCES)))
PROJECT_OBJECTS = $(patsubst %.cpp, $(BUILD_DIR)\\%.o, $(notdir $(PROJECT_SOURCES))) 
GLFW_OBJECTS = $(patsubst %.c, $(BUILD_DIR)\\%.o, $(notdir $(GLFW_SOURCES)))
GLEW_OBJECTS = $(patsubst %.c, $(BUILD_DIR)\\%.o, $(notdir $(GLEW_SOURCES)))

# Generate list of include directories for each section
GLFW_INCLUDES = -I$(GLFW_PATH)\include -I$(GLFW_PATH)\src
GLEW_INCLUDES = -I$(GLEW_PATH)\include 

# The shell command finds all the paths to the .h files. The dir command removes the .h filename from the
# the paths. The patsubst command removes the trailing \ so it ends in \Inc instead of \Inc\. The sort command
# removes all the duplicate paths. The addprefix adds -I to the start of each path
CPP_LIB_HEADERS = $(shell dir /b /s "E:\Software_Libraries\Personal\Cpp\*.h")
CPP_LIB_INCLUDES = $(addprefix -I, $(sort $(patsubst %\Inc\, %\Inc, $(dir $(CPP_LIB_HEADERS)))))

C_LIB_HEADERS = $(shell dir /b /s "E:\Software_Libraries\Personal\C\*.h")
C_LIB_INCLUDES = $(addprefix -I, $(sort $(patsubst %\Inc\, %\Inc, $(dir $(C_LIB_HEADERS)))))

PROJECT_HEADERS = $(shell dir /b /s "*.h")
PROJECT_INCLUDES = $(addprefix -I, $(sort $(patsubst %\Inc\, %\Inc, $(dir $(PROJECT_HEADERS)))))

# Compile list containing all .cpp object and include files
INCLUDES = \
$(PROJECT_INCLUDES) \
$(CPP_LIB_INCLUDES) \
$(C_LIB_INCLUDES) \
$(GLFW_INCLUDES) \
$(GLEW_INCLUDES) \
$(LIB_STB_INCLUDES) \
$(LIB_SP_INCLUDES)

# Optimisation flags
OPT=-Og
CPP_COMPILER=g++
C_COMPILER=gcc
DEBUG_MODE=-g
FLAGS=-Wall -Wextra

CPP_LIB_FLAGS = $(FLAGS) $(DEBUG_MODE) $(INCLUDES) $(OPT) -lm
C_LIB_FLAGS = $(FLAGS) $(DEBUG_MODE) $(C_LIB_INCLUDES) $(OPT) -lm
LIB_STB_FLAGS = $(FLAGS) $(LIB_STB_INCLUDES)
PROJECT_FLAGS = $(FLAGS) $(DEBUG_MODE) $(INCLUDES) $(OPT) -lm
GLEW_FLAGS = $(DEBUG_MODE) -DGLEW_STATIC $(GLEW_INCLUDES) $(OPT)
GLFW_FLAGS = $(DEBUG_MODE) $(GLFW_INCLUDES) $(OPT)

ALL_OBJECTS=\
$(GLEW_OBJECTS) \
$(GLFW_OBJECTS) \
$(CPP_LIB_OBJECTS) \
$(C_LIB_OBJECTS) \
$(PROJECT_OBJECTS) \
$(LIB_SP_OBJECTS_BUILD_DIR) \
$(STB_OBJECTS_BUILD_DIR)

all:

# The first prerequisite checks if the build directory exists. The next prerequistes will
# try recompile the .o files if they do not exist or if the sources files have been updated
# since the last builld. The line $(wordlist 2,$(words $^),$^) just ensures that only the 
# prerequistes after $(BUILD_DIR) are used
all: $(BUILD_DIR) $(ALL_OBJECTS)
	$(CPP_COMPILER) -o $(EXECUTABLE) $(wordlist 2, $(words $^), $^) -lgdi32 -lopengl32 -lsetupapi

# Recipe to create build folder
$(BUILD_DIR):
	mkdir $@

# Generic recipes for creating the given object files from the required .cpp files
$(GLEW_OBJECTS): $(GLEW_SOURCES)
	$(C_COMPILER) $(GLEW_FLAGS) -c -o $@ $(filter %\$(notdir $(basename $@)).c, $^)

$(GLFW_OBJECTS): $(GLFW_SOURCES)
	$(C_COMPILER) $(GLFW_FLAGS) -c -o $@ $(filter %\$(notdir $(basename $@)).c, $^) -D _GLFW_WIN32

# The filtering is there to reconstruct the path to the .cpp file so the .o file
# can be compiled properly
$(CPP_LIB_OBJECTS): $(filter %\$(notdir $(basename $@)).cpp, $(CPP_LIB_SOURCES)) $(CPP_LIB_HEADERS)
	$(CPP_COMPILER) $(CPP_LIB_FLAGS) -c -o $@ $(filter %\$(notdir $*).cpp, $(CPP_LIB_SOURCES))

# The filtering is there to reconstruct the path to the .cpp file so the .o file
# can be compiled properly
$(C_LIB_OBJECTS): $(filter %\$(notdir $(basename $@)).c, $(C_LIB_SOURCES)) $(C_LIB_HEADERS)
	$(C_COMPILER) $(C_LIB_FLAGS) -c -o $@ $(filter %\$(notdir $*).c, $(C_LIB_SOURCES))

# The filtering is there to reconstruct the path to the .cpp file so the .o file
# can be compiled properly
$(PROJECT_OBJECTS): $(filter %\$(notdir $(basename $@)).cpp, $(PROJECT_SOURCES)) $(PROJECT_HEADERS)
	$(CPP_COMPILER) $(PROJECT_FLAGS) -c -o $@ $(filter %\$(notdir $*).cpp, $(PROJECT_SOURCES))

# Stuff for Libserial port and stb
$(LIB_SP_OBJECTS_BUILD_DIR):
	$(C_COMPILER) $(FLAGS) -c -o $@ $(LIB_SP_PATH)/$(notdir $(basename $@)).c

$(STB_OBJECTS_BUILD_DIR):
	$(C_COMPILER) $(LIB_STB_FLAGS) -c -o $@ $(STB_PATH)/$(notdir $(basename $@)).c


# Speed up next build process by only removing non 3rd party .o files as the 3rd party .o files
# will never change
clean:
	@del /f $(PROJECT_OBJECTS) $(C_LIB_OBJECTS) $(CPP_LIB_OBJECTS)
	
# Remove the entire build directory
fullclean:
	@rmdir /s /q $(BUILD_DIR)

# Run executable
run:
	.\$(EXECUTABLE)

.PHONY: run clean fullclean