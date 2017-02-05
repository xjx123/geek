local ski = require("ski")
local log = require("log")
local js = require("cjson.safe")

local mqtt_trans_map = {}

local function transfer(topic, payload)
	local f = mqtt_trans_map[topic]
	if f then
		local map = f(payload)
		return map
	end

	return payload
end

-- 监听database数据库的改变信息
mqtt_trans_map["a/ac/database_sync"] = function(map)
	if not (map and map.pld) then
		return
	end

	local arr = map.pld
	if not (arr.cmd == "dbsync" and arr.data.device) then -- 只接收device表的改变信息
		return
	end
	local data = arr.data.device

	local map = data.set or data.add
	if not map then
		return
	end

	return {pld = {cmd = "dbsync_device", data = map}}
end

-- 监听ap上报version
mqtt_trans_map["a/ac/query/version"] = function(map)
	if not (map and map.pld) then
		return
	end

	local data = map.pld
	local group, apid, ver = data[1], data[2], data[3]
	if not (group and apid and #apid == 17 and ver) then
		return
	end

	return {pld = {cmd = "version", data = map}}
end

-- 监听ap上报注册信息
mqtt_trans_map["a/ac/cfgmgr/register"] = function(map)
	local cmd = map.pld
	if not (map and cmd) then
		return
	end

	local data = cmd.data
	if not data then
		return
	end

	return {pld = {cmd = "register", data = map}}
end

mqtt_trans_map["a/ac/cfgmgr/modify"] = function(map)
	local cmd = map.pld
	if not (map and cmd) then
		return
	end

	return {pld = {cmd = "modify", data = map}}
end

mqtt_trans_map["a/ac/cfgmgr/network"] = function(map)
	if not (map and map.pld) then
		return
	end

	return {pld = {cmd = "network", data = map}}
end

mqtt_trans_map["a/ac/query/will"] = function(map)
	if not (map and map.apid and map.group) then
		return
	end

	return {pld = {cmd = "will", data = map}}
end

mqtt_trans_map["a/ac/query/connect"] = function(map)
	local map = map.pld
	if not (map and map.group and map.apid and map.data) then
		return
	end

	return {pld = {cmd = "connect", data = map}}
end

mqtt_trans_map["a/ac/query/noupdate"] = function(map)
	local map = map.pld
	if not (map and map.group and map.apid) then
		return
	end

	return {pld = {cmd = "noupdate", data = map}}
end

return {transfer = transfer}