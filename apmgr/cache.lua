-- xjx

local fp 	= require("fp")
local ski 	= require("ski")
local log 	= require("log")
local batch	= require("batch")
local nos 	= require("luanos")
local js 	= require("cjson.safe")
local rpccli 	= require("rpccli")
local simplesql = require("simplesql")

local mqtt, dbrpc, simple
local clear_map = {}

local function init(p)
	mqtt = p
	dbrpc = rpccli.new(mqtt, "a/ac/database_srv")
	simple = simplesql.new(dbrpc)
end

local device_cache
local function check_device()
	if device_cache and #device_cache ~= 0 then
		return
	end

	local sql = string.format("select devid,version from device")
	local rs, e = simple:mysql_select(sql)
	if not rs then
		log.error("check_device fail %s", e)
		rs = {}
	end

	device_cache = rs
end

local function get_version(devid)
	if not device_cache then
		check_device()
	end

	local tarr = {}
	for _, item in ipairs(device_cache or {}) do
		local k = item.devid
		tarr[k] = item.version
	end

	return tarr[devid]
end

function clear()
	device_cache = nil
end

return {
	init			= init,
	clear			= clear,
	check_device	= check_device,
	get_version		= get_version,
}