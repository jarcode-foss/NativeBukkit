SHELL := /bin/bash
LUA_EXEC = "lua"
# build file
BUILD_FILE = "build.lua"

.PHONY: all

all:
	$(LUA_EXEC) $(BUILD_FILE) all

install:
	$(LUA_EXEC) $(BUILD_FILE) install

clean:
	$(LUA_EXEC) $(BUILD_FILE) clean

debug:
	$(LUA_EXEC) $(BUILD_FILE) debug
