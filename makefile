# https://stackoverflow.com/questions/2908057/can-i-compile-all-cpp-files-in-src-to-os-in-obj-then-link-to-binary-in
# -w -O0 -g2 
# -w -O0 -g2 

SRC_DIR := src
OBJ_DIR := obj
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
LDFLAGS  := -rdynamic -O0 -w -g2 -std=c++17 -pthread -ldl
CPPFLAGS := -rdynamic -O0 -w -g2 -std=c++17 -pthread
CXXFLAGS := 

ck: $(OBJ_FILES)
	g++ $(LDFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	g++ $(CPPFLAGS) $(CXXFLAGS) -Iinclude -c -o $@ $<
	
clear:
	rm -rf obj/*