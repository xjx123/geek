-- xjx

local fp  	= require("fp")
local ski	= require("ski")
local log	= require("log")
local fs	= require("ski.fs")
local misc	= require("ski.misc")
local js	= require("cjson.safe")
local apmlib	= require("apmlib")
local online	= require("online")
local rpccli	= require("rpccli")
local register	= require("register")
local checkout	= require("checkout")
local simplesql = require("simplesql")
local board		= require("cfg.board")
local chkfirm	= require("chkfirm")
local cache		= require("cache")

local read = fs.readfile
local tcp_map	= {}		-- cmd from webui
local mqtt_map	= {}		-- cmd from mqtt
local mqtt, dbrpc, simple
local port, host, actype = 80
local dl_path = "/usr/share/apmgr/download.sh"

local function init(p)
	mqtt	= p
	dbrpc	= rpccli.new(mqtt, "a/ac/database_srv")
	simple	= simplesql.new(dbrpc)

	online.init(mqtt)
	checkout.init(mqtt)
	register.init(mqtt)
	cache.init(mqtt)
	misc.execute("[ -d /tmp/www/rom ] || mkdir -p /tmp/www/rom/")
end

local function mqtt_publish(t, s)
	if not (t and s) then
		return nil, "invalid publish"
	end

	return mqtt:publish(t, s)
end

-- 全量更新ap信息
local function replace_notify(apid_map)
	for apid, map in pairs(apid_map) do
		local p = {
			mod = "a/local/cfgmgr",
			pld = {cmd = "replace", data = map and map or {}},
		}

		local _ = log.real_enable() and log.real1("replace_notify map:%s", js.encode(map))
		local _ = mqtt_publish("a/ap/" .. apid, js.encode(p)) or log.error("replace_notify publish fail")
	end
end

-- 批量更新ap信息
local function update_ap(apid_map)
	for apid, map in pairs(apid_map) do
		local p = {
			mod = "a/local/cfgmgr",
			pld = {cmd = "update", data = map and map or {}},
		}

		local _ = log.real_enable() and log.real1("update_ap map:%s", js.encode(map))
		local _ = mqtt_publish("a/ap/" .. apid, js.encode(p)) or log.error("update_ap publish fail")
	end
end

mqtt_map["dbsync_device"] = function(map)
	cache.clear()
	for i, devid in ipairs(map) do
		local map_file = checkout.find_ap_config(devid)
		local _ = update_ap({[devid] = map_file})

		if i % 10 == 0 then
			ski.sleep(0.05)
		end
	end
end

mqtt_map["version"] = function(map)
	local data = map.pld
	local group, apid, ver = data[1], data[2], data[3]

	local _ = log.real_enable() and log.real1("version map:%s", js.encode(map))

	local r, e = mqtt:publish("a/ac/status", js.encode({pld = {cmd = "set_login_time", data = {apid = apid, force = false}}}))

	local _ = r or log.error("version set_login_time fail %s %s", apid, e)
	online.set_online(group, apid)

	local curver = cache.get_version(apid)
	if not curver then
		local p = {
			mod = map.mod,
			pld = {cmd = "delete", data = {"version+"}},
		}
		local _ = mqtt_publish(map.tpc, js.encode(p)) or log.error("version publish fail")
		return
	end

	if ver == curver then
		return
	end

	replace_notify({[apid] = checkout.find_ap_config(apid)})
end

mqtt_map["register"] = function(map)
	local cmd = map.pld

	local res, apid_map = register.register(cmd)
	local p = {
		mod = map.mod,
		seq = map.seq,
		pld = res,
	}

	local topic, payload = map.tpc, js.encode(p)
	local ac_devid = board.get_devid()
	topic = cmd.data[1] == ac_devid and "a/ap/" .. ac_devid or topic

	local _ = mqtt_publish(topic, payload) or log.error("register publish fail")
	local _ = apid_map and replace_notify(apid_map)
end

local modify_map = {}
modify_map["upgrade"] = function(map)
	local group, arr = map.group, map.arr.data
	for _, apid in ipairs(arr) do
		online.set_upgrade(group, apid)
		local p = {
			mod = "a/local/cfgmgr",
			pld = {cmd = "upgrade", data = { host = "auto" }},
		}
		local _ = mqtt_publish("a/ap/" .. apid, js.encode(p)) or log.error("upgrade publish fail")
	end
end

mqtt_map["modify"] = function(map)
	local cmd = map.pld
	local func = modify_map[cmd.cmd]

	if func then
		return func(cmd.data)
	end
end

mqtt_map["network"] = function(map)
	local m = register.set_network(map.pld)
	local _ = m and update_ap(m)
end

mqtt_map["will"] = function(map)
	log.debug("will apid:%s", map.apid)
	online.set_offline(map.group, map.apid)
end

mqtt_map["connect"] = function(map)
	local group, apid, data = map.group, map.apid, map.data

	log.debug("connect apid:%s", apid)
	local r, e = mqtt:publish("a/ac/status", js.encode({pld = {cmd = "set_login_time", data = {apid = apid, force = true}}}))
	local _ = r or log.error("connect set_login_time fail %s %s", apid, e)

	online.set_online(group, apid)
end

mqtt_map["noupdate"] = function(map)
	online.set_noupgrade(map.group, map.apid)
end

tcp_map["apm_fwexec"] = function(p)
	if not (p and p.apcmd and p.apids) then
		return nil, "error apids"
	end

	local data = {cmd = p.apcmd, data = {}}
	local apids = js.decode(p.apids)
	if not apids then
		return nil, "no apids"
	end
	for _, apid in ipairs(apids) do
		local map = {
			mod = "a/local/cfgmgr",
			pld = {cmd = "exec", data = data},
		}

		mqtt_publish("a/ap/" .. apid, js.encode(map))
	end

	return true
end

tcp_map["apm_fwupgrade"] = function(map)
	if not map.data then
		return nil, "invalid data"
	end

	map.data = js.decode(map.data)
	if not map.data then
		return nil, "invalid data"
	end
	local flag = false
	local dir = "/tmp/www/rom/"
	if not fs.access(dir) then
		return nil, "cannot upgrade"
	end

	local up_map = {}
	local files = fs.scandir(dir) or {}
	for _, filename in pairs(files) do
		local tmp_map = {}
		for _, v in pairs(map.data) do
			table.insert(tmp_map, v)
		end

		local aptype = filename:match("(.+)%.version$")
		if aptype then
			flag = true
			local sql = string.format("select devid from apstate where firmware like '%%%s%%'", aptype)
			local apids, e = simple:mysql_select(sql)
			if not apids then
				return nil, e
			end

			for i = 1, #tmp_map, 1 do
				local tmp = table.remove(tmp_map, 1)
				for _, v in pairs(apids) do
					if v.devid == tmp then
						table.insert(up_map, tmp)
					end
				end
			end
		end
	end
	map.data = up_map
	if not flag then
		return nil, "cannot upgrade"
	end
	local p = {
		mod = "a/local/cfgmgr",
		pld = {cmd = "upgrade", data = { group = "default", arr = map}},
	}

	mqtt_publish("a/ac/cfgmgr/modify", js.encode(p))
	return true
end

local is_download = false
local function dlver()
	is_download = true
	misc.execute("rm /etc/config/thin_version.json")
	if not fs.stat(dl_path) then
		return nil, "download list failed"
	end
	misc.execute(string.format("sh %s apver %s %s %s", dl_path, actype, host, port))
	is_download = false
end

local count = 0
tcp_map["apm_fwlist"] = function(p)
	local narr = {}
	local aptype_map = {}
	local arr_insert = function(aptype_map)
		for k, item in pairs(aptype_map) do
			table.insert(narr, {cur = item.old, new = item.new})
		end
	end

	if is_download then
		return nil, "it's downloading"
	end

	local dir = "/tmp/www/rom"
	if not fs.access(dir) then
		count = count + 1
		misc.execute("[ -d /tmp/www/rom ] || mkdir -p /tmp/www/rom/")
		return nil, "downloading"
	end

	-- 添加已下载固件包版本
	local files = fs.scandir(dir) or {}
	for _, filename in pairs(files) do
		local aptype = filename:match("(.+)%.version$")
		if aptype then
			local map = aptype_map[aptype] or {}
			map.old = read(dir .. "/" .. filename):match("(.-)\n")
			aptype_map[aptype] = map
		end
	end

	host, port, actype = chkfirm.check_firmware(dbrpc)
	if not (host and actype and port) or host == "" then
		arr_insert(aptype_map)
		return narr
	end

	ski.go(dlver)

	-- 添加下载固件列表版本
	local path = "/etc/config/thin_version.json"
	if not fs.stat(path) then
		count = count + 1
		if count == 4 then
			count = 0
			misc.execute("touch /etc/config/thin_version.json")
		end
		return nil, "downloading"
	end
	local map = js.decode(read(path) or "{}") or {}
	for aptype, item in pairs(map) do
		local map = aptype_map[aptype] or {}
		map.new = item.version
		aptype_map[aptype] = map
	end

	arr_insert(aptype_map)

	return narr
end

tcp_map["apm_logget"] = function(map)
	if not (map and map.devid) then
		return nil, "invalid logget"
	end

	local self_devid = board.get_devid()
	local apid = map.devid
	if apid == self_devid then
		local s1 = read("/tmp/log/log.current") or ""
		local s2 = read("/tmp/log/lua.error") or ""
		local s = table.concat({s1, "-------------------------------------", s2}, "\n\n")
		return s
	end
	local timeout = 5
	local r, e = mqtt:query_log("a/local/cfgmgr", map, timeout)
	return r
end

return {
	init			= init,
	dispatch_tcp	= apmlib.gen_dispatch_tcp(tcp_map),
	dispatch_mqtt	= apmlib.gen_dispatch_mqtt(mqtt_map),
}
