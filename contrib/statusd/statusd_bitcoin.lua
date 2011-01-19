-- Bitcoin (฿) daemon monitor

-- %bitcoin_speed - current speed in khash/s
-- %bitcoin_balance - current balance

-- by Voker57 <voker57@gmail.com>
-- Public domain

local defaults={
    update_interval=30 * 1000
}

local settings=table.join(statusd.get_config("bitcoin"), defaults)

local bitcoin_timer

local function get_bitcoin_speed()
	local hps = io.popen("bitcoind gethashespersec 2>/dev/null")
	if not hps then
		return "N/A"
	end
	r = hps:read()
	if not r then
		return "N/A"
	else
		return string.format("%0.2f", r / 1000)
	end
end

local function get_bitcoin_balance()
	local bl = io.popen("bitcoind getbalance 2>/dev/null")
	if not bl then
		return "N/A"
	end
	r = bl:read()
	if not r then
		return "N/A"
	else
		return string.format("%0.2f", r)
	end
end

local function update_bitcoin()
	local bitcoin_speed = get_bitcoin_speed()
	local bitcoin_balance = get_bitcoin_balance()
	statusd.inform("bitcoin_balance", bitcoin_balance)
	statusd.inform("bitcoin_speed", bitcoin_speed)
	bitcoin_timer:set(settings.update_interval, update_bitcoin)
end

-- Init
bitcoin_timer=statusd.create_timer()
update_bitcoin()
