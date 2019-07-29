CXX = g++
CXXFLAGS = -std=c++11 -g -Wall
LDLIBS = -lOpenImageIO

OBJ_DIR = bin
LIB_DIR = -L/opt/lib/oiio/lib
INC_DIR = -I/opt/lib/oiio/include

SRC = main.cpp camera.cpp film.cpp renderer.cpp #renderglobals.cpp
OBJECTS = $(SRC:%.cpp=$(OBJ_DIR)/%.o)
EXEC = moeru_kirin

all: init $(OBJECTS) $(EXEC)

$(EXEC):
	$(CXX) $(CXXFLAGS) $(LIB_DIR) -o $@ $(OBJECTS) $(LDLIBS)

$(OBJ_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INC_DIR) -c $< -o $@

init:
	@mkdir -p $(OBJ_DIR)

clean:
	rm $(OBJECTS) $(EXEC)
