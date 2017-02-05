local log = require("log")
local js = require("cjson.safe")

local function gen_dispatch_tcp(tcp_map)
	return function(cmd)
		local f = tcp_map[cmd.cmd]
		if f then
			return true, f(cmd)
		end
	end
end

local function gen_dispatch_mqtt(mqtt_map)
	return function(cmd)
		local f = mqtt_map[cmd.cmd]
		if f then
			return true, f(cmd.data)
		end
	end
end

local function once(dbrpc, code, p, flag)
	local r, e = dbrpc:once(code, p)
	if r then
		return r
	end

	local _ = r or log.error("%s %s", flag, e)
	return nil, "mysql error"
end

return {
	once 				= once,
	gen_dispatch_tcp 	= gen_dispatch_tcp,
	gen_dispatch_mqtt 	= gen_dispatch_mqtt,
}