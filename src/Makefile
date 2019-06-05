CXX=g++

BUILD_DIR=../build/
TARGETS=$(addprefix $(BUILD_DIR), simulator.o networkObject.o packetHandler.o\
	switch.o endpoint.o interface.o flow.o link.o packet.o)

.PHONY: all clean

all: $(BUILD_DIR) $(TARGETS)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BUILD_DIR)%.o: %.cpp
	$(CXX) -o $@ -c $<

$(BUILD_DIR)simulator.o: simulator.cpp switch.h

$(BUILD_DIR)networkObject.o: networkObject.cpp networkObject.h

$(BUILD_DIR)packetHandler.o: packetHandler.cpp packetHandler.h networkObject.h interface.h

$(BUILD_DIR)switch.o: switch.cpp switch.h packetHandler.h networkObject.h

$(BUILD_DIR)endpoint.o: endpoint.cpp endpoint.h packetHandler.h networkObject.h

$(BUILD_DIR)interface.o: interface.cpp interface.h networkObject.h

$(BUILD_DIR)flow.o: flow.cpp flow.h networkObject.h

$(BUILD_DIR)link.o: link.cpp link.h networkObject.h

$(BUILD_DIR)packet.o: packet.cpp packet.h endpoint.h networkObject.h

clean:
	$(RM) $(TARGETS)
