# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Produce verbose output by default.
VERBOSE = 1

# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/fangshao/CPP/Project/windgent

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/fangshao/CPP/Project/windgent/build

# Include any dependencies generated for this target.
include CMakeFiles/test_bytearray.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/test_bytearray.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/test_bytearray.dir/flags.make

CMakeFiles/test_bytearray.dir/tests/test_bytearray.cc.o: CMakeFiles/test_bytearray.dir/flags.make
CMakeFiles/test_bytearray.dir/tests/test_bytearray.cc.o: ../tests/test_bytearray.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/fangshao/CPP/Project/windgent/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/test_bytearray.dir/tests/test_bytearray.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) -D__FILE__=\"tests/test_bytearray.cc\" $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/test_bytearray.dir/tests/test_bytearray.cc.o -c /home/fangshao/CPP/Project/windgent/tests/test_bytearray.cc

CMakeFiles/test_bytearray.dir/tests/test_bytearray.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test_bytearray.dir/tests/test_bytearray.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) -D__FILE__=\"tests/test_bytearray.cc\" $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/fangshao/CPP/Project/windgent/tests/test_bytearray.cc > CMakeFiles/test_bytearray.dir/tests/test_bytearray.cc.i

CMakeFiles/test_bytearray.dir/tests/test_bytearray.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test_bytearray.dir/tests/test_bytearray.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) -D__FILE__=\"tests/test_bytearray.cc\" $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/fangshao/CPP/Project/windgent/tests/test_bytearray.cc -o CMakeFiles/test_bytearray.dir/tests/test_bytearray.cc.s

# Object files for target test_bytearray
test_bytearray_OBJECTS = \
"CMakeFiles/test_bytearray.dir/tests/test_bytearray.cc.o"

# External object files for target test_bytearray
test_bytearray_EXTERNAL_OBJECTS =

../bin/test_bytearray: CMakeFiles/test_bytearray.dir/tests/test_bytearray.cc.o
../bin/test_bytearray: CMakeFiles/test_bytearray.dir/build.make
../bin/test_bytearray: ../lib/libwindgent.so
../bin/test_bytearray: /home/fangshao/CPP/Project/yaml-cpp/build/libyaml-cpp.so
../bin/test_bytearray: CMakeFiles/test_bytearray.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/fangshao/CPP/Project/windgent/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../bin/test_bytearray"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_bytearray.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/test_bytearray.dir/build: ../bin/test_bytearray

.PHONY : CMakeFiles/test_bytearray.dir/build

CMakeFiles/test_bytearray.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/test_bytearray.dir/cmake_clean.cmake
.PHONY : CMakeFiles/test_bytearray.dir/clean

CMakeFiles/test_bytearray.dir/depend:
	cd /home/fangshao/CPP/Project/windgent/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/fangshao/CPP/Project/windgent /home/fangshao/CPP/Project/windgent /home/fangshao/CPP/Project/windgent/build /home/fangshao/CPP/Project/windgent/build /home/fangshao/CPP/Project/windgent/build/CMakeFiles/test_bytearray.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/test_bytearray.dir/depend
