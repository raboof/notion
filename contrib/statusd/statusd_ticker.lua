-- Authors: Matus Telgarsky <mtelgars@andrew.cmu.edu>
-- License: Unknown
-- Last Changed: Unknown
--
-- little ticker boondoggle
--
-- Selects amongst a set of specified commands and scrolls them not only line by line
--  but also within each line.  Waits and marks ends of lines, beginning of command.
--  Should work great with an rss reader though I didn't care to try.
--
-- Author: Matus Telgarsky < mtelgars at andrew dot cmu dot edu >
--
-- I couldn't sleep and felt like learning lua.
--
--

local settings = {
    timer = nil,
    commands = {
        'printf "hello world"',
        'fortune',
        'df --si',
    },
    random = true,
    line_len = 50,
    step = 2,
    new = {
        hint = "critical",
        interval = 1 * 1000,
    },
    line = {
        hint = "important",
        interval = 1 * 1000,
    },
    scroll = { 
        hint = "normal",
        interval = 0.375 * 1000,
    },
    update_fun = nil, --forward decl hack
}

local message = {
    fd = nil,
    s = nil,
    len = nil,
    pos = nil,
}

local function ticker_timer(style)
    statusd.inform("ticker_hint", settings[style].hint)
    settings.timer:set(settings[style].interval, settings.update_fun)
end

local function ticker_line_init()
    message.len = string.len(message.s)
    message.pos = 0
end

local function ticker_single()
    return settings.commands[1]
end

local function ticker_random()
    --don't do same twice, uniformly distribute chances across others
    local newpos = math.random(1, settings.commands.count-1)
    if newpos >= settings.commands.pos then
        newpos = newpos + 1
    end

    settings.commands.pos = newpos
    return settings.commands[settings.commands.pos]
end

local function ticker_rotate()
    local c = settings.commands[settings.commands.pos]

    if settings.commands.pos == settings.commands.count then
        settings.commands.pos = 1
    else
        settings.commands.pos = settings.commands.pos + 1
    end

    return c
end

local function ticker_update()
    if message.fd == nil then
        message.fd = io.popen(settings.commands.get(), 'r')
        message.s = message.fd:read()
        if message.s then --XXX this is a bug workaround! 
            ticker_line_init()
        end
        ticker_timer('new')
    elseif message.s == nil then
        message.s = message.fd:read()
        if message.s == nil then
            message.fd:close()
            message.fd = nil
        else
            ticker_line_init()
        end
        ticker_timer('line')
    elseif message.pos + settings.line_len >= message.len then
        message.s = nil --hang out at the end of a line
        ticker_timer('line')
    else
        message.pos = message.pos + settings.step
        ticker_timer('scroll')
    end

    if message.s ~= nil then
        statusd.inform("ticker", 
            string.sub(message.s, message.pos, message.pos + settings.line_len))
    end
end

if statusd ~= nil then
    settings.timer = statusd.create_timer()
    statusd.inform("ticker_template", string.rep('x', settings.line_len))

    settings.update_fun = ticker_update
    settings.commands.count = #(settings.commands)
    if settings.commands.count == 1 then
        settings.commands.get = ticker_single
    elseif settings.random then
        settings.commands.pos = 1
        settings.commands.get = ticker_random
    else 
        settings.commands.pos = 1
        settings.commands.get = ticker_rotate
    end

    ticker_update()
end
