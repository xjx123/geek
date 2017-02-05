-- xjx

local log	= require("log")
local js	= require("cjson.safe")
local rpccli	= require("rpccli")
local memfile	= require("memfile")
local simplesql = require("simplesql")

local max_upgrade_time = 590
local estat = {offline = 0, online = 1, upgrade = 2}
local mf_online = memfile.ins("online_ap")
local mf_upgrade = memfile.ins("upgrade_ap")
local mqtt, dbrpc, simple

local function init(p)
	mqtt = p
	dbrpc = rpccli.new(mqtt, "a/ac/database_srv")
	simple = simplesql.new(dbrpc)
end

local function get_current_state(group, apid)
	local map = mf_online:get(group) or mf_online:set(group, {}):get(group)
	return map, map[apid]
end

local function get_upgrade_state(group, apid)
	local map = mf_upgrade:get(group) or mf_upgrade:set(group, {}):get(group)
	return map, map[apid]
end

local function set_upgrade_state(group, apid, map, time)
	map[apid] = time
	mf_upgrade:set(group, map):save()
end

-- 0：离线	1：在线	2：升级
local function set_state(group, apid, map, state)
	assert(group and apid and map and state)
	map[apid] = state
	mf_online:set(group, map):save()

	local _ = log.real_enable() and log.real1("set_state apid:%s, set_state:%s", apid, state)
	local r, e = mqtt:publish("a/ac/status", js.encode({pld = {cmd = "set_state", data = {apid = apid, state = state}}}))
	local _ = r or log.error("set_state fail %s %s", apid, e)
end

local function set_online(group, apid)
	local map, state = get_current_state(group, apid)

	if not state then
		return set_state(group, apid, map, 1)
	end

	if state == estat.online then
		return
	end

	local umap, time = get_upgrade_state(group, apid)
	if not time then
		set_state(group, apid, map, 1)
		return set_upgrade_state(group, apid, umap)
	end

	local d = os.time() - time

	if d >= 0 and d < max_upgrade_time then
		return
	end

	set_state(group, apid, map, 1)
	set_upgrade_state(group, apid, umap)
end

local function set_offline(group, apid)
	local map, state = get_current_state(group, apid)
	if state and state ~= estat.online then
		return	-- 离线或者升级，不修改
	end

	set_state(group, apid, map, 0)
end

local function set_upgrade(group, apid)
	local map, state = get_current_state(group, apid)
	set_state(group, apid, map, 2)

	-- 设置升级状态
	local map, state = get_upgrade_state(group, apid)
	set_upgrade_state(group, apid, map, os.time())
end

local function set_noupgrade(group, apid)
	local map, state = get_current_state(group, apid)
	set_state(group, apid, map, 1)

	-- 取消升级状态
	local map = get_upgrade_state(group, apid)
	set_upgrade_state(group, apid, map)
end

return {
	init			= init,
	set_online		= set_online,
	set_offline		= set_offline,
	set_upgrade		= set_upgrade,
	set_noupgrade	= set_noupgrade,
}
