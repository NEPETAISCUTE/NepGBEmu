RM+=-rfv

CC     := gcc
CFLAGS := -Wall -O3
DFLAGS := -Wall -g
LIBS   := -lraylib -lpthread -ldl -lc -lm
LD     := gcc

SRC     := src
INCLUDE := include
BUILD   := build
DEPSDIR := $(BUILD)

BINARY := nepEmuGB
DBGBINARY := $(BINARY)_dbg

SOURCESPATHS := $(wildcard $(SRC)/*.c)
OBJECTS      := $(SOURCESPATHS:$(SRC)/%.c=$(BUILD)/%.o)
DEPS         := $(SOURCESPATHS:$(SRC)/%.c=$(DEPSDIR)/%.d)

build: $(BINARY)

$(BINARY): $(OBJECTS)
	@echo Linking object into $@
	@$(LD) $(DFLAGS) -I$(INCLUDE) $(OBJECTS) -o $@ $(LIBS) 

install: $(BINARY)
	@cp -uv $(BINARY) /usr/bin/

-include $(DEPSDIR)/$(DEPS)
$(BUILD)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	@echo Compiling $< into $@
	@$(CC) $(DFLAGS) -I$(INCLUDE) -c $< -o $@ -MMD

clean:
	@$(RM) $(BUILD)

wipe: clean
	@$(RM) $(BINARY)

re: wipe build

run: $(BINARY)
	./$(BINARY)

rerun: re run
