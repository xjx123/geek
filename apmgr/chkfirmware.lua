local fp 		= require("fp")
local log		= require('log')
local ski		= require("ski")
local fs		= require("ski.fs")
local apmlib	= require("apmlib")
local rpccli	= require("rpccli")
local chkfirm	= require('chkfirm')
local misc		= require("ski.misc")
local simplesql = require("simplesql")
local js 		= require("cjson.safe")

local tcp_map = {}
local mqtt, dbrpc, simple
local read = fs.readfile
local port, host, actype = 80
local latest, nonexist, status = {}, {}
local dir = "/tmp/www/rom/"
local dl_path = "/usr/share/apmgr/download.sh"
local thin_version = "/etc/config/thin_version.json"

local function compare_big(aptype, cur, late)
	local tmp = {["ap152"] = 1, ["ap143"] = 1, ["ap121"] = 1}
	local tmp_tm, tmp_v, tmp_tm1, tmp_v1
	if tmp[aptype] then
		tmp_tm, tmp_v = cur:match("%w+%.(%d+)%-[Vv](.+)")
		tmp_tm1, tmp_v1 = late:match("%w+%.(%d+)%-[Vv](.+)")
	elseif aptype == "QM1439" then
		tmp_tm, tmp_v = cur:match("%.(%d+)"), "0"
		tmp_tm1, tmp_v1 = late:match("%.(%d+)"), "0"
	else
		tmp_tm, tmp_v = cur:match("%w+%-[Vv](.+)%-(%d+)")
		tmp_tm1, tmp_v1 = late:match("%w+%-[Vv](.+)%-(%d+)")
	end
	if tmp_v1 > tmp_v then
		return false
	end

	if tmp_v1 == tmp_v and tmp_tm1 > tmp_tm then
		return false
	end

	if tmp_v1 == tmp_v and tmp_tm1 == tmp_tm then
		return true
	end
end

local function tmp_space()
	misc.execute("[ -e /tmp/space.tmp ] && rm -f /tmp/space.tmp;df -h |grep '/tmp$' |awk '{print $4}'> /tmp/space.tmp")
	local avail = read("/tmp/space.tmp")
	if not avail then
		return nil, "download failed"
	end
	local count = avail:match('(.+)M')
	if not count then
		count = 0
	end
	return count
end

local function download_ap_firmware(down_list)
	local do_download = function(aptype, item)
		local md5, patharr = item.md5, item.path
		if not (md5 and patharr and #patharr > 0) then
			return nil, "download failed"
		end
		if not fs.stat(dl_path) then
			return nil, "download failed"
		end
		local cmd = string.format("sh %s apdl %s %s %s %s %s ", dl_path, aptype, host, port, item.version, md5)
		local narr = {}
		for _, v in ipairs(patharr) do
			table.insert(narr, "'" .. v .. "'")
		end
		cmd = cmd .. table.concat(narr, " ")
		misc.execute(cmd)
	end

	for aptype, item in pairs(down_list) do
		if item.path then
			local patharr = item.path
			if patharr and #patharr > 0 then
				do_download(aptype, item)
			end
		end
	end
end

local function isexist(aptype)
	local files = fs.scandir(dir) or {}
	for _, filename in pairs(files) do
		local aptypes = filename:match("(.+)%.version$")
		if aptypes then
			if aptypes == aptype then
				return true
			end
		end
	end
	return false
end

local function del(aptype)
	local str = string.format("rm -f /tmp/www/rom/%s*", aptype)
	misc.execute(str)
end

-- 1. 在线ap全升级到最新版本
-- 2. 下载的固件包型号和在线的所有ap不匹配
local function useless()
	local sql = string.format("select firmware from apstate where state='1'")
	local rs, e = simple:mysql_select(sql)
	if not rs then
		return -1
	end

	local arr = {}
	for _, ver in pairs(rs) do
		table.insert(arr, ver.firmware)
	end

	local aparr, flag = {}, false
	local files = fs.scandir(dir) or {}
	for _, filename in pairs(files) do
		local aptype = filename:match("(.+)%.version$")
		if aptype then
			local path = string.format("/tmp/www/rom/%s.version", aptype)
			if not fs.stat(path) then
				return -1
			end
			local s = read(path)
			if not s then
				return -1
			end

			local over = s:match("(.-)\n")
			for _, v in pairs(arr) do
				if v:match(aptype) then
					local tmp_v = v:sub(v:find(aptype), -1)
					if compare_big(aptype, tmp_v, over) then
						aparr[aptype] = 1
					end
					flag = true
				end
			end
			if not flag then
				aparr[aptype] = 1
			end
			flag = false
		end
	end

	return aparr
end

local function get_new_version()
	local map = {}
	if not fs.stat(thin_version) then
		return nil, "download failed"
	end

	local s = read(thin_version)
	if not s then
		return nil, "download failed"
	end

	local map = js.decode(s) or {}

	return map
end

local function get_list_version(param)
	local sql = string.format("select firmware from apstate where devid in ('%s')", table.concat(param,"','"))
	local rs, e = simple:mysql_select(sql)
	if not rs then
		return nil, e
	end

	local arr = {}
	for _, ver in pairs(rs) do
		arr[ver.firmware] = 1
	end
	return arr
end

local function get_candidate(cur_ver, new_ver)
	local arr = {}
	for version, _ in pairs(cur_ver) do
		for aptype, item in pairs(new_ver) do
			if version:match(aptype) then
				local version = version:sub(version:find(aptype), -1)
				if not compare_big(aptype, version, item.version) then
					arr[aptype] = item
				end
			end
		end
	end

	return arr
end

local function get_info(param, cur_ver, new_ver)
	local flag = false
	latest, nonexist = {}, {}
	for version, _ in pairs(cur_ver) do
		for aptype, item in pairs(new_ver) do
			if version:match(aptype) then
				local version = version:sub(version:find(aptype), -1)
				if compare_big(aptype, version, item.version) then
					table.insert(latest, version)
				end
			flag = true
			end
		end
		if not flag then
			table.insert(nonexist, version)
		end
		flag = false
	end
end

local function start_download(p)
	status = "predownloading"
	host, port, actype = chkfirm.check_firmware(dbrpc)
	if not (host and actype and port) or host == "" then
		status = "failed"
		return "failed"
	end

	local str = string.format("/tmp/www/rom4ac/%s.version", actype)
	if fs.stat(str) then
		status = "failed"
		return "failed"
	end

	if not fs.access(dir) then
		status = "failed"
		return "failed"
	end

	local param = {}
	if p and p.data then
		param = js.decode(p.data)
		if not param then
			status = "failed"
			return "failed"
		end
	end

	-- 获取勾选ap设备的当前版本
	local cur_ver, e = get_list_version(param)
	if not (cur_ver and type(cur_ver) == 'table' ) then
		status = "failed"
		return "failed"
	end

	-- 获取可升级的固件列表
	local new_ver, e = get_new_version()
	if not (new_ver and type(new_ver) == 'table' ) then
		status = "failed"
		return "failed"
	end

	-- 提示不用升级 勾选的ap固件版本：{ap152-V1.0-20160708} 未下载列表：{ap152-V1.0-20160708}
	-- 提示当前无可用下载的版本 勾选的ap固件版本：{ap152-V1.0-20160708} 未下载列表：{ap143-V1.0-20160708}
	get_info(param, cur_ver, new_ver)

	-- 获取候选列表
	local candy = get_candidate(cur_ver, new_ver)

	-- 获取无用固件包，准备删除
	local up_latest = useless()
	if type(up_latest) == "table" then
		for k, _ in pairs(up_latest) do
			del(k)
		end
	end

	-- 最后准备升级的固件列表
	local down_list = {}
	for aptype, item in pairs(candy) do
		if not isexist(aptype) then
			down_list[aptype] = item
		end
	end

	status = "downloading"
	local count = tmp_space() or "0"
	local _ = tonumber(count) > 32 and download_ap_firmware(down_list)
	status = "success"
end

tcp_map["apm_fwdl"] = function(p)
	local prompt = {}
	prompt.latest = latest
	prompt.nonexist = nonexist
	if status == "downloading" then
		return js.encode(prompt)
	end

	if status == "predownloading" then
		return "predownloading"
	end

	if not status then
		ski.go(start_download, p)
		return "predownloading"
	end

	if status == "success" or status == "failed" then
		status = nil
		return js.encode(prompt)
	end
end

local function init(p)
	mqtt = p
	dbrpc = rpccli.new(mqtt, "a/ac/database_srv")
	simple = simplesql.new(dbrpc)
end

return { init = init, dispatch_tcp = apmlib.gen_dispatch_tcp(tcp_map)}
