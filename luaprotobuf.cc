extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include <string>
#include <iostream>
#include <memory>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/arena.h>

using namespace std;
using namespace google::protobuf;
using namespace google::protobuf::compiler;

class MuFiErCo : public compiler::MultiFileErrorCollector
{
public:
    void AddError(const string & filename, int line, int column, const string & message){
        printf("Err: %s\n", message.c_str());
    }
    void AddWarning(const string & filename, int line, int column, const string & message){
        printf("Warn: %s\n", message.c_str());
    }

};

static Arena arena;
static DiskSourceTree source_tree;
static MuFiErCo error_mist;
static Importer *imp;
static DynamicMessageFactory dmf;


/*******************************************************/
/*  create a new message class object                  */
/*******************************************************/
static Message* createMessage(const string &name) {

	const DescriptorPool *pool = imp->pool();
	const Descriptor* descriptor = pool->FindMessageTypeByName(name);

	Message* message = NULL;
	if (descriptor) {
		const Message* prototype = dmf.GetPrototype(descriptor);
		if (prototype) {
			message = prototype->New();
		}
	}

	assert(message);
	return message;
}

/*******************************************************/
/*  destroy an existing message class object           */
/*******************************************************/
static void releaseMessage(Message* msg) {
	if (NULL != msg) {
		msg->Clear();
		delete msg;
		msg = NULL;
	}
}

/*******************************************************/
/*  parse a message from a lua table                   */
/*******************************************************/
#define CASE_FIELD_TYPE(cpptype, protobuf_method, lua_method) \
	case FieldDescriptor::CPPTYPE_##cpptype: { \
		reflection->protobuf_method(msg, field, lua_method(L, -1));  \
		break;\
	} 

#define SWITCH_FIELD_TYPE(method_prefix, arg1, arg2) \
switch(field->cpp_type()) {\
	CASE_FIELD_TYPE(INT32, method_prefix##Int32, lua_tointeger)\
	CASE_FIELD_TYPE(INT64, method_prefix##Int64, lua_tointeger)\
	CASE_FIELD_TYPE(UINT32, method_prefix##UInt32, lua_tointeger)\
	CASE_FIELD_TYPE(UINT64, method_prefix##UInt64, lua_tointeger)\
	CASE_FIELD_TYPE(FLOAT, method_prefix##Float, lua_tonumber)\
	CASE_FIELD_TYPE(DOUBLE, method_prefix##Double, lua_tonumber)\
	CASE_FIELD_TYPE(BOOL, method_prefix##Bool, lua_toboolean)\
	CASE_FIELD_TYPE(STRING, method_prefix##String, lua_tostring)\
	case FieldDescriptor::CPPTYPE_ENUM: { \
		const string name(lua_tostring(L, -1));\
		const EnumValueDescriptor *valueDesc = field->enum_type()->FindValueByName(name);\
		reflection->method_prefix##Enum(msg, field, valueDesc);  \
		break;\
	}\
	case FieldDescriptor::CPPTYPE_MESSAGE: {\
		const string &name = field->message_type()->full_name();\
		Message* submsg = createMessage(name);\
		parseMessage(L, submsg);\
		reflection->method_prefix##AllocatedMessage(msg, arg1, arg2);\
		break;\
	}\
	default: {\
			printf("Unknown cpptype!\n");\
			releaseMessage(msg);\
			break;\
	}\
}

// have to set the message table at top of lua_state* L
static void parseMessage(lua_State* L, Message* msg) {

	const Descriptor* descriptor = msg->GetDescriptor();
	const Reflection* reflection = msg->GetReflection();
	for (int i = 0; i < descriptor->field_count(); i++) {
		const FieldDescriptor* field = descriptor->field(i);
		const string& name = field->name();

		lua_getfield(L, -1, name.c_str());

		if (lua_isnil(L, -1) && field->is_required()) {
			printf("Error: a required field in message is missing!\n");
			releaseMessage(msg);
			return;
		}

		if (!lua_isnil(L, -1)) {
			if (field->is_repeated()) {
				lua_pushnil(L);
				while (lua_next(L, -2) != 0) {
					SWITCH_FIELD_TYPE(Add, field, submsg)
					lua_pop(L, 1);
				}

			} else {
				SWITCH_FIELD_TYPE(Set, submsg, field)
			}
		}

		lua_pop(L, 1);
	}
}

#undef SWITCH_FIELD_TYPE //(method_prefix, arg1, arg)
#undef CASE_FIELD_TYPE //(cpptype, protobuf_method, lua_method)

/*******************************************************/
/*  write a protobuf class object into a lua table     */
/*******************************************************/
#define CASE_FIELD_TYPE(cpptype, lua_method, protobuf_method, ...) \
	case FieldDescriptor::CPPTYPE_##cpptype:\
			lua_method(L, reflection->protobuf_method((*pmsg), field, ##__VA_ARGS__));\
		break;

#define SWITCH_FIELD_TYPE(prefix, ...) \
	switch(field->cpp_type()) { \
		CASE_FIELD_TYPE(INT32, lua_pushinteger, prefix##Int32, ##__VA_ARGS__) \
		CASE_FIELD_TYPE(INT64, lua_pushinteger, prefix##Int64, ##__VA_ARGS__) \
		CASE_FIELD_TYPE(UINT32, lua_pushinteger, prefix##UInt32, ##__VA_ARGS__) \
		CASE_FIELD_TYPE(UINT64, lua_pushinteger, prefix##UInt64, ##__VA_ARGS__) \
		CASE_FIELD_TYPE(FLOAT, lua_pushnumber, prefix##Float, ##__VA_ARGS__) \
		CASE_FIELD_TYPE(DOUBLE, lua_pushnumber, prefix##Double, ##__VA_ARGS__) \
		CASE_FIELD_TYPE(BOOL, lua_pushboolean, prefix##Bool, ##__VA_ARGS__) \
		case FieldDescriptor::CPPTYPE_ENUM: \
			lua_pushstring(L, reflection->prefix##Enum((*pmsg), field, ##__VA_ARGS__)->name().c_str());\
			break; \
		case FieldDescriptor::CPPTYPE_STRING: \
			lua_pushstring(L, reflection->prefix##String((*pmsg), field, ##__VA_ARGS__).c_str());\
			break; \
		case FieldDescriptor::CPPTYPE_MESSAGE: \
			writeMessage(L, &reflection->prefix##Message((*pmsg), field, ##__VA_ARGS__)); \
			break; \
		default: \
			cout << "Unknown field" << endl; \
			break; \
	}

static void writeMessage(lua_State* L, const Message* pmsg){
        const Descriptor* descriptor = pmsg->GetDescriptor();
        const Reflection* reflection = pmsg->GetReflection();

	lua_createtable(L, 0, descriptor->field_count());
        for (int i=0; i<descriptor->field_count(); i++){
                const FieldDescriptor* field = descriptor->field(i);
		const string& name = field->name();

		lua_pushstring(L, name.c_str());

		bool flag = true;
		if (field->is_repeated()) {
			lua_createtable(L, reflection->FieldSize((*pmsg), field), 0);
			for (int j = 0; j < reflection->FieldSize((*pmsg), field); j++) {
				lua_pushinteger(L, j+1);
				SWITCH_FIELD_TYPE(GetRepeated, j)	
				lua_settable(L, -3);
			}
			if (reflection->FieldSize((*pmsg), field) == 0) {
				lua_pop(L, 1);
				flag = false;
			}

		} else {
			//if (reflection->HasField((*pmsg), field)) {
				SWITCH_FIELD_TYPE(Get)
			//} else {
			//	flag = false;	
			//}
		}
		if (flag) {
			lua_settable(L, -3);
		} else {
			lua_pop(L, 1);
		}
        }
}

#undef SWITCH_FIELD_TYPE
#undef CASE_FIELD_TYPE


/*****************************************************************************************/
/*  lua interface for encoding a lua table message into a binary protobuf string buffer  */
/*****************************************************************************************/
static int encode(lua_State *L){
	const string name(lua_tostring(L, -2));
	Message* msg = createMessage(name);
	parseMessage(L, msg);

	string buffer;
	if (!msg->SerializeToString(&buffer)) {
		printf("Failed to serialize message: nbaproto.%s\n", lua_tostring(L, -2));
	}
	lua_pushlstring(L, buffer.c_str(), buffer.length());
	
	releaseMessage(msg);

	return 1;
}

/*****************************************************************************************/
/*  lua interface for decoding a binary protobuf string buffer into a lua table message  */
/*****************************************************************************************/
static int decode(lua_State *L) {
	const string name(lua_tostring(L, -2));
	Message* msg = createMessage(name);
	size_t len;
	const char *s = lua_tolstring(L, -1, &len);
	string buffer(s, len);
	if(!msg->ParseFromString(buffer)) {
		printf("Failed to parse message: nbaproto.%s\n", lua_tostring(L, -2));
	}

	writeMessage(L, msg);

	releaseMessage(msg);

	return 1;
}

static int register_file(lua_State *L) {
	const char *dir = lua_tostring(L, -2);
	const char *filename = lua_tostring(L, -1);
	source_tree.MapPath("", dir);
	imp = Arena::Create<Importer>(&arena, &source_tree, &error_mist);
	imp->Import(filename);
	return 0;
}

static luaL_Reg mylibs[] = {
	{"register_file", register_file},
	{"decode", decode},
	{"encode", encode},
	{NULL, NULL},
};

extern "C" int luaopen_luaprotobuf(lua_State* l){
	luaL_newlib(l, mylibs);
	return 1;
}


