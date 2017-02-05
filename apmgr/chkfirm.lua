-- xjx

local ski	= require("ski")
local log	= require("log")
local fs	= require("ski.fs")
local apmlib = require("apmlib")
local misc	= require("ski.misc")
local js	= require("cjson.safe")

local port, host, actype = 80
local read = fs.readfile
local firmware_path = "/etc/openwrt_release"

local function get_config(dbrpc)
	local h, p, t
	if not fs.stat(firmware_path) then
		return nil, "invalid config"
	end
	local s = read(firmware_path)
	if not s then
		return nil, "invalid config"
	end

	local fmap = {}
	local rs, rs1 = s:match("^(.-)=\'(.-)\'")
	fmap[rs] = rs1
	if not fmap[rs] then
		return nil, "invalid config"
	end
	t = fmap[rs]

	local code = [[
		local ins = require("mgr").ins()
		local js = require("cjson.safe")
		local myconn = ins.myconn

		local sql = string.format("select v from kv where k='cloud_account_info'")
		local ret, e = myconn:select(sql)
		if not ret then
			return nil, e
		end
		return ret
	]]
	local ret, e = apmlib.once(dbrpc, code, rawset({}, "cmd", nil))
	if not ret then
		return nil, e
	end
	local cmap = js.decode(ret[1].v)
	if not (cmap and cmap.ac_host) then
		return nil, "invalid data"
	end

	h = cmap and cmap.ac_host
	p = 80
	return h, p, t
end

function check_firmware(dbrpc)
	local h, p, a = get_config(dbrpc)
	if not (h and a and p) then
		return nil, "error data"
	end
	return h, p, a
end
return { check_firmware = check_firmware }
