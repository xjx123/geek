-- xjx

local ski	= require("ski")
local log	= require("log")
local pkey 	= require("key")
local const = require("constant")
local js	= require("cjson.safe")
local rpccli	= require("rpccli")
local simplesql = require("simplesql")

local keys = const.keys
local mqtt, dbrpc, simple
local function init(p)
	mqtt	= p
	dbrpc	= rpccli.new(mqtt, "a/ac/database_srv")
	simple	= simplesql.new(dbrpc)
end

-- 格式化ap配置为ap可解析的格式
local function format_apconfig(map, devtype)
	local kvmap = {}

	-- common
	local mc = {
		devdesc		= keys.c_desc,
		gw			= keys.c_gw,
		ip			= keys.c_ip,
		mask		= keys.c_mask,
		dns			= keys.c_dns,
		distribute	= keys.c_distr,
		ac_host		= keys.c_ac_host,
		ac_port		= keys.c_ac_port,
		mode		= keys.c_mode,
		scan_chan	= keys.c_scan_chan,
		hbd_time	= keys.c_hbd_time,
		hbd_cycle	= keys.c_hbd_cycle,
		nml_cycle	= keys.c_nml_cycle,
		nml_time	= keys.c_nml_time,
		mnt_time	= keys.c_mnt_time,
		mnt_cycle	= keys.c_mnt_cycle,
		version		= keys.c_version,
		radios		= keys.c_barr,
	}
	for _, item in ipairs(map.common) do
		for k, v in pairs(item) do
			if mc[k] then
				local key = pkey.short(mc[k])
				kvmap[key] = key and v
				if key == pkey.short(keys.c_version) then
					kvmap[key] = string.gsub(kvmap[key], "%-*:*%s*", "")
				end
			end
		end
	end

	-- opt
	local mo = {
		ienable		= keys.c_rs_iso,
		mult		= keys.c_ag_rs_mult,
		rate		= keys.c_ag_rs_rate,
		inspeed		= keys.c_rs_inspeed,
		enable		= keys.c_ag_rs_switch,
		country		= keys.c_g_country,
		debug		= keys.c_g_debug,
		ledctrl		= keys.c_g_ledctrl,
		account		= keys.c_account,
		reboot		= keys.c_ag_reboot,
		ld_switch	= keys.c_g_ld_switch,
		rdo_cycle	= keys.c_ag_rdo_cycle,
		upload_log	= keys.c_upload_log,
		sta_cycle	= keys.c_ag_sta_cycle,
		g_recover	= keys.c_g_recover,
	}
	for _, item in ipairs(map.global) do
		local key = pkey.short(mo[item.k])
		kvmap[key] = item.v
	end

	-- radio
	for _, item in ipairs(map.radio) do
		local band = item.band
		item.band = nil
		item.devid = nil

		for k, v in pairs(item) do
			kvmap[string.format("a#%s#%s", band, k)] = v
		end
	end
	kvmap.devtype = devtype == "ac" and devtype or nil

	-- wlan
	local wlanid_arr = {}
	if not map.wlan then
		kvmap["a#wlanids"] = js.encode(wlanid_arr)
		return {fix = kvmap, wlan = {}}
	end

	local wlan_map = {}
	for _, item in ipairs(map.wlan) do
		local m_wlan = {}
		local wlanid = string.format("%05d", item.wlanid)
		table.insert(wlanid_arr, wlanid)
		local p = string.format("w#%s#", wlanid)
		m_wlan[p .. "wssid"]		= item.ssid
		m_wlan[p .. "wstate"]		= item.enable
		m_wlan[p .. "whide"]		= item.hide
		m_wlan[p .. "wencry"]		= item.encrypt
		m_wlan[p .. "wpasswd"]		= item.password
		m_wlan[p .. "wband"]		= item.band
		if devtype == "ac" then
			m_wlan[p .. "wnetwork"] = item.network
		else
			m_wlan[p .. "wvlanenable"]	= tostring(item.vlan_enable)
			m_wlan[p .. "wvlanid"]		= tostring(item.vlanid)
		end
		wlan_map[wlanid] = m_wlan
	end
	kvmap["a#wlanids"] = js.encode(wlanid_arr)

	return {fix = kvmap, wlan = wlan_map}
end

-- 从数据库中取对应ap的数据
local function fetch_raw_config(devid)
	local sql = string.format("select * from device where devid='%s'", devid)
	local carr, e = simple:mysql_select(sql)
	if not (carr and #carr == 1) then
		log.error("select device fail %s", e)
		return nil, e or "no dev:" .. devid
	end

	local sql = string.format("select * from radio where devid='%s'", devid)
	local barr, e = simple:mysql_select(sql)
	if not (barr and #barr > 0) then
		log.error("select radio fail %s", e)
		return nil, e or "no radio:" .. devid
	end

	local sql = string.format("select * from kv where k='inspeed' or k='ienable' or k='mult' or k='rate' or k='enable' or k='country' or k='debug' or k='ledctrl' or k='account' or k='reboot' or k='ld_switch' or k='rdo_cycle' or k='upload_log' or k='sta_cycle' or k='g_recover'")
	local acarr, e = simple:mysql_select(sql)
	if not (acarr and #acarr == 15) then
		log.error("select kv fail %s", e)
		return nil, e
	end

	local sql = string.format("select * from wlan a, wlan2ap b where b.devid='%s' and a.wlanid=b.wlanid order by a.wlanid", devid)
	local warr, e = simple:mysql_select(sql)
	local _ = warr or log.error("select wlan,wlan2ap fail %s", e)

	local map = {
		common	= carr,
		radio	= barr,
		wlan	= warr,
		global	= acarr,
	}
	local devtype = carr[1].devtype
	local kvmap = format_apconfig(map, devtype)

	return kvmap
end

-- 返回ap可解析的map
local function find_ap_config(devid)
	local map_config = fetch_raw_config(devid)
	if not map_config then
		return nil, "error map_config"
	end

	log.debug("find_ap_config ok devid:%s", devid)
	return map_config
end

-- 将ap上传的格式信息转换成数据库中的格式
local function transformat(devid, map, devtype)
	local md = {
		{k = keys.c_desc,		desc = "devdesc"},
		{k = keys.c_gw,			desc = "gw"},
		{k = keys.c_ip,			desc = "ip"},
		{k = keys.c_mask,		desc = "mask"},
		{k = keys.c_dns,		desc = "dns"},
		{k = keys.c_distr,		desc = "distribute"},
		{k = keys.c_ac_host,	desc = "ac_host"},
		{k = keys.c_ac_port,	desc = "ac_port"},
		{k = keys.c_mode,		desc = "mode"},
		{k = keys.c_scan_chan,	desc = "scan_chan"},
		{k = keys.c_hbd_time,	desc = "hbd_time"},
		{k = keys.c_hbd_cycle,	desc = "hbd_cycle"},
		{k = keys.c_nml_cycle,	desc = "nml_cycle"},
		{k = keys.c_nml_time,	desc = "nml_time"},
		{k = keys.c_mnt_time,	desc = "mnt_time"},
		{k = keys.c_mnt_cycle,	desc = "mnt_cycle"},
		{k = keys.c_version,	desc = "version"},
		{k = keys.c_barr,		desc = "radios"},
	}

	local mt = {
		{k = keys.c_bswitch,	desc = "bswitch"},
		{k = keys.c_proto,		desc = "proto"},
		{k = keys.c_power,		desc = "power"},
		{k = keys.c_chanid,		desc = "chanid"},
		{k = keys.c_bandwidth,	desc = "bandwidth"},
		{k = keys.c_usrlimit,	desc = "usrlimit"},
		{k = keys.c_ampdu,		desc = "ampdu"},
		{k = keys.c_amsdu,		desc = "amsdu"},
		{k = keys.c_beacon,		desc = "beacon"},
		{k = keys.c_dtim,		desc = "dtim"},
		{k = keys.c_leadcode,	desc = "leadcode"},
		{k = keys.c_remax,		desc = "remax"},
		{k = keys.c_rts,		desc = "rts"},
		{k = keys.c_shortgi,	desc = "shortgi"},
	}

	local kvmap = {}
	local ap = {}
	for _, item in pairs(md) do
		local key	= pkey.short(item.k)
		local desc	= item.desc
		ap[desc]	= map[key]
	end

	kvmap.ap = ap
	kvmap.ap.devid = devid
	kvmap.ap.devtype = devtype and devtype or "ap"

	local mr = {"2g", "5g"}
	local mb = {{BAND = "2g"}, {BAND = "5g"}}
	for i = 1, #mb do
		local radio, band = mr[i], mb[i]
		local t = {}
		for _, item in pairs(mt) do
			local key	= pkey.short(item.k, band)
			local desc	= item.desc
			t[desc]		= map[key]
		end

		kvmap[radio] = t
		kvmap[radio]["band"] = radio
		kvmap[radio]["devid"] = devid
	end

	return kvmap
end

-- 校验 mac
local mac_pattern = (function()
	local arr = {}
	for i = 1, 6 do table.insert(arr, "[0-9a-fA-F][0-9a-fA-F]") end
	return string.format("^%s$", table.concat(arr, ":"))
end)()

-- 校验 ip
local ip_pattern = (function()
	local arr = {}
	for i = 1, 4 do table.insert(arr, "[0-9][0-9]?[0-9]?") end
	return string.format("^%s$", table.concat(arr, "%."))
end)()

return {
	init			= init,
	ip_pattern		= ip_pattern,
	mac_pattern		= mac_pattern,
	transformat		= transformat,
	find_ap_config	= find_ap_config,
}
