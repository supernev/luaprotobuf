# luaprotobuf
Simple Lua binding on protobuf-cpp, encode/decode pb messages to/from Lua table.

Requirements: 
	1. Protobuf headers and libs.
	2. Lua header files.

How to build: 
	make
	make test

To use your own protobuf path
	make PB_INC=/your/include/path PB_LIB=/your/lib/path
