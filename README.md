# luaprotobuf

Simple Lua binding on protobuf-cpp, encode/decode pb messages to/from Lua table.

## Requirements:

- Protobuf headers and libs.
- Lua header files.

## How to build

```
make
```
## To use your own protobuf path
Makefile use /usr/local/include, /usr/local/lib by default for searching headers and lib files for Lua and Protobuf
Use LUA_INC, PB_INC and PB_LIB for custom Lua, Protobuf path.

```
make LUA_INC=/your/include/path PB_INC=/your/include/path PB_LIB=/your/lib/path
```

## Run test

```
make test
```

## Example

See test.lua

