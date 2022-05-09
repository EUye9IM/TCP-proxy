CXX			:= g++
CXX_FLAGS	:= -Wall -Werror
EXTRA_NAME	:= cpp

SRC			:= src
BUILD		:= build
EXECUTABLE	:= server client

INCLUDE		:= -I include 
LIB			:=
LIBRARIES	:=

SRC_COMMON	:= $(wildcard $(SRC)/common/*.$(EXTRA_NAME) $(SRC)/common/**/*.$(EXTRA_NAME))
OBJ_COMMON	:= $(patsubst $(SRC)/%.$(EXTRA_NAME),$(BUILD)/%.o,$(SRC_COMMON))
SRC_SERVER	:= $(wildcard $(SRC)/server/*.$(EXTRA_NAME) $(SRC)/server/**/*.$(EXTRA_NAME))
OBJ_SERVER	:= $(patsubst $(SRC)/%.$(EXTRA_NAME),$(BUILD)/%.o,$(SRC_SERVER))
SRC_CLIENT	:= $(wildcard $(SRC)/client/*.$(EXTRA_NAME) $(SRC)/client/**/*.$(EXTRA_NAME))
OBJ_CLIENT	:= $(patsubst $(SRC)/%.$(EXTRA_NAME),$(BUILD)/%.o,$(SRC_CLIENT))

RM			:=
ifeq ($(OS),Windows_NT)
# PLATFORM="Windows" 
 RM			:= del /s /q /f
else
#  PLATFORM="Unix-Like"
 RM			:= rm -rf
endif



.PHONY: all
all:$(EXECUTABLE)
	@echo Success.

server:$(OBJ_COMMON) $(OBJ_SERVER)
	@echo Linking $@
	@$(CXX) $(CXX_FLAGS) $(INCLUDE) $^ $(LIB) $(LIBRARIES) -o $@

client:$(OBJ_COMMON) $(OBJ_CLIENT)
	@echo Linking $@
	@$(CXX) $(CXX_FLAGS) $(INCLUDE) $^ $(LIB) $(LIBRARIES) -o $@

$(BUILD)/%.o: $(SRC)/%.$(EXTRA_NAME)
	@echo Compiling $@
ifeq ($(OS),Windows_NT)
	@if not exist $(dir $@) mkdir "$(dir $@)
else
#  PLATFORM="Unix-Like"
	@mkdir -p $(dir $@)
endif
	@$(CXX) $(CXX_FLAGS) $(INCLUDE) -c $< -o $@

.PHONY: clean
clean:
	@echo remove $(BUILD)
	@$(RM) $(BUILD)
	@echo remove $(EXECUTABLE)
	@$(RM) $(EXECUTABLE)
