local protobuf = require "luaprotobuf"

local message = {
		a = 1,
		b = {"22", "33"},
		c = {
			{code = 111, msg = "222"}, 
			{code = 1111, msg = "2222"}
		},
		phone_type1 = "HOME",
		phone_type2 = "MOBILE",
		phone_type3 = "WORK",
		phone_types = {"MOBILE", "HOME", "WORK"}
	}

local function test()

	protobuf.register_file("", "test.proto")

	
	local buffer = protobuf.encode("test.Test", message)
	local msg = protobuf.decode("test.Test", buffer)

	assert(msg.a == 1)
	assert(#msg.b == 2)
	assert(msg.b[1] == "22")
	assert(msg.b[2] == "33")
	assert(#msg.c == 2)
	assert(msg.c[1].code == 111)
	assert(msg.c[1].msg == "222")
	assert(msg.c[2].code == 1111)
	assert(msg.c[2].msg == "2222")

	assert(msg.phone_type1 == "HOME")
	assert(msg.phone_type2 == "MOBILE")
	assert(msg.phone_type3 == "WORK")
	assert(#msg.phone_types == 3)
	assert(msg.phone_types[1] == "MOBILE")
	assert(msg.phone_types[2] == "HOME")
	assert(msg.phone_types[3] == "WORK")
end

test()

print("SUCCESS.")
