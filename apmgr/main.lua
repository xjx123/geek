-- xjx

local ski	= require("ski")
local log	= require("log")
local fs	= require("ski.fs")
local js	= require("cjson.safe")
local common		= require("common")
local sandcproxy	= require("sandcproxy")
local modport		= require("modport")

js.encode_keep_buffer(false)
js.encode_sparse_array(true)

local mqtt_chan, mqtt
local safe_require = common.safe_require
local read = fs.readfile

local transform = safe_require("transform")

local modules = {
	apevent		=	safe_require("apevent"),
	chkfirmware	=	safe_require("chkfirmware"),
}
local srv_port = modport.getport("apmgr")

local function dispatch_mqtt_loop()
	local f = function(map)
		local match, r, e, s
		local cmd, topic, seq = map.pld, map.mod, map.seq
		for _, mod in pairs(modules) do
			local f = mod.dispatch_mqtt
			if f and f(cmd) then
				local _ = mod and seq and mqtt:publish(topic, js.encode({seq = seq, pld = {r, e}}))
				return
			end
		end
	end

	local r, e
	while true do
		r, e = mqtt_chan:read()		assert(r, e)
		f(r)
	end
end

local function start_sand_server()
	local pld, cmd, map, r, e
	local unique = "a/ac/apmgr"

	local on_message = function(topic, payload)
		local map = js.decode(payload)
		if not map then
			return
		end

		local map = transform.transfer(topic, map)
		if not map then
			return
		end

		local r, e = mqtt_chan:write(map)	assert(r, e)
	end

	local args = {
		log = log,
		unique = unique,
		clitopic = {unique,
					"a/ac/query/version",
					"a/ac/cfgmgr/register",
					"a/ac/cfgmgr/modify",
					"a/ac/cfgmgr/network",
					"a/ac/cfgmgr/query",
					"a/ac/query/will",
					"a/ac/query/connect",
					"a/ac/query/noupdate",
					"a/ac/database_sync",
					},
		srvtopic = {unique .. "_srv"},
		on_message = on_message,
		on_disconnect = function(st, err) log.fatal("disconnect %s %s", st, err) end,
	}

	return sandcproxy.run_new(args)
end

local function loop_check_debug()
	local path = "/tmp/debug_apmgr"
	while true do
		if fs.stat(path) then
			local s = read(path), fs.unlink(path)
			local _ = (#s == 0 and log.real_stop or log.real_start)(s)
		end
		ski.sleep(5)
	end
end

local function start_tcp_server()
	local tcp = require("ski.tcp")
	local new_request = require("request").new

	local srv = tcp.listen("127.0.0.1", srv_port) 	assert(srv)
	while true do
		local cli, e = srv:accept() 			assert(cli, e)
		new_request(cli, function(data)
			local m = js.decode(data)
			if m and m.cmd then
				for _, mod in pairs(modules) do
					local f = mod.dispatch_tcp
					if f then
						local find, r, e = f(m)
						if find then
							return r and {status = 0, data = r} or {status = 1, data = e}
						end
					end
				end
			end
			return {status = 1, data = "invalid cmd"}
		end):run()
	end
end

local function main()
	log.setmodule("ap")
	mqtt_chan = ski.new_chan(100)
	mqtt = start_sand_server()
	for _, mod in pairs(modules) do
		mod.init(mqtt)
	end

	ski.go(start_tcp_server)
	local _ = ski.go(dispatch_mqtt_loop), ski.go(loop_check_debug)
end

ski.run(main)
