# https://stackoverflow.com/questions/2908057/can-i-compile-all-cpp-files-in-src-to-os-in-obj-then-link-to-binary-in
# -w -O0 -g2 
# -w -O0 -g2 

SRC_DIR := src
OBJ_DIR := obj
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
LDFLAGS  := -rdynamic -O3 -std=c++17 -pthread -fpermissive
CPPFLAGS := -rdynamic -O3 -std=c++17 -pthread -fpermissive
CXXFLAGS := 
CAAFLAGS := -ldl

ck: $(OBJ_FILES)
	g++ $(LDFLAGS) -o $@ $^ $(CAAFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	g++ $(CPPFLAGS) $(CXXFLAGS) -Iinclude -c -o $@ $< $(CAAFLAGS)
	
clear:
	rm -rf obj/*