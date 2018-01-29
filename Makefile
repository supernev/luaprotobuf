CC ?= gcc
CFLAGS = -g -O2 -Wall
CXXFLAGS = -lstdc++
SHARED = -shared -fPIC

LUA_INC ?= /usr/local/include
PB_INC ?= /usr/local/include
PB_LIB ?= /usr/local/lib

.PHONY : all
all : luaprotobuf.so

# Static linking libprotobuf.a, make sure ld can find libprotobuf.a
luaprotobuf.so : luaprotobuf.cc 
	$(CC) $(CFLAGS) $(CXXFLAGS) $(SHARED) luaprotobuf.cc -o luaprotobuf.so -I$(LUA_INC) -I$(PB_INC) -L$(PB_LIB) -Wl,-Bstatic -lprotobuf -Wl,-Bdynamic 

clean :
	rm -f luaprotobuf.so

test : 
	lua test.lua
