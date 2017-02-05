-- xjx

local fp	= require("fp")
local ski	= require("ski")
local log	= require("log")
local pkey 	= require("key")
local fs	= require("ski.fs")
local misc	= require("ski.misc")
local const = require("constant")
local js	= require("cjson.safe")
local online	= require("online")
local rpccli	= require("rpccli")
local simplesql = require("simplesql")
local checkout	= require("checkout")
local cache		= require("cache")

local mqtt, dbrpc, simple

local function init(p)
	mqtt = p
	dbrpc = rpccli.new(mqtt, "a/ac/database_srv")
	simple = simplesql.new(dbrpc)
end

local function register(cmd)
	local data = cmd.data
	if not data then
		return
	end

	local devid, group = data[1], data[2]
	if not (devid and #devid == 17 and devid:match("^%x%x:%x%x:%x%x:%x%x:%x%x:%x%x$")) then
		return
	end

	local r, e = mqtt:publish("a/ac/status", js.encode({pld = {cmd = "set_login_time", data = {apid = devid, force = false}}}))
	if not r then
		log.error("register set_login_time fail %s %s", devid, e)
		return
	end
	online.set_noupgrade(group, devid)

	local curver = cache.get_version(apid)
	local exist = curver

	if cmd.cmd == "check" then 	-- 检查devid的配置是否存在
		if not exist then
			log.debug("check devid %s not exist", devid)
			return 0
		end

		local ver = data[3]
		if ver == curver then
			return 1
		end

		return 1, {[devid] = checkout.find_ap_config(devid)}
	end

	if exist then
		log.debug("already exists %s", devid)
		return 1
	end

	if not (cmd.cmd == "upload") then
		return
	end

	local kvmap = checkout.transformat(devid, data[3], cmd.devtype)
	local r, e = mqtt:query("a/ac/cfgmgr_srv", {cmd = "ap_add", data = kvmap})
	if not r then
		log.error("register ap_add fail %s %s", devid, e)
		return
	end

	return 1, {[devid] = checkout.find_ap_config(devid)}
end

local function set_network(map)
	if not (map and map.apid and map.data) then
		return nil, "nil netwrok map"
	end

	local devid, kvmap = map.apid, map.data
	local sql = string.format("select ip, mask, gw, distribute from device where devid='%s'", devid)
	local rs, e = simple:mysql_select(sql)
	if not (rs and #rs > 0) then
		log.error("set_network not exist %s", devid)
		return
	end

	local transformat = function(map)
		local new = {
			ip = map["a#ip"],
			mask = map["a#mask"],
			gw = map["a#gw"],
			distribute = map["a#distr"],
		}
		return new
	end
	local kvmap = transformat(kvmap)

	local ip, mask, gw = kvmap.ip, kvmap.mask, kvmap.gw
	if not (ip:find(checkout.ip_pattern) or mask:find(checkout.mac_pattern) or gw:find(checkout.ip_pattern)) then
		return nil, "invalid ip or mask or gw"
	end

	if fp.same(kvmap, rs[1]) then
		return {[devid] = checkout.find_ap_config(devid)}
	end

	kvmap.version = os.date("%Y-%m-%d %H:%M:%S")
	kvmap.devid = devid

	local r, e = mqtt:query("a/ac/cfgmgr_srv", {cmd = "set_network", data = kvmap})
	local _ = r or log.error("set_network fail %s %s", devid, e)

	log.debug("set_netwrok ok!")
	return {[devid] = checkout.find_ap_config(devid)}
end

return {
	init		= init,
	register	= register,
	set_network	= set_network,
}
