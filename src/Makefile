CXX=g++
CXXFLAGS=-Wall -Wextra -pedantic -g

BUILD_DIR=../build/
TARGETS=$(addprefix $(BUILD_DIR), simulator.o networkObject.o packetHandler.o\
	switch.o endpoint.o interface.o flow.o link.o packet.o manager.o)

.PHONY: all clean test

all: $(BUILD_DIR) $(TARGETS)

test:
	make -C tests
	make -C tests clean

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BUILD_DIR)%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<

$(BUILD_DIR)simulator.o: simulator.cpp switch.h manager.h

$(BUILD_DIR)networkObject.o: networkObject.cpp networkObject.h

$(BUILD_DIR)packetHandler.o: packetHandler.cpp packetHandler.h networkObject.h interface.h

$(BUILD_DIR)switch.o: switch.cpp switch.h packetHandler.h networkObject.h manager.h

$(BUILD_DIR)endpoint.o: endpoint.cpp endpoint.h packetHandler.h networkObject.h

$(BUILD_DIR)interface.o: interface.cpp interface.h networkObject.h packet.h link.h packetHandler.h

$(BUILD_DIR)flow.o: flow.cpp flow.h networkObject.h

$(BUILD_DIR)link.o: link.cpp link.h networkObject.h packet.h interface.h

$(BUILD_DIR)packet.o: packet.cpp packet.h endpoint.h networkObject.h

$(BUILD_DIR)manager.o: manager.cpp manager.h

clean:
	$(RM) $(TARGETS)
