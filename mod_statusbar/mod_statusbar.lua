--
-- ion/mod_statusbar/mod_statusbar.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004-2005.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

-- This is a slight abuse of the _LOADED variable perhaps, but library-like 
-- packages should handle checking if they're loaded instead of confusing 
-- the user with require/include differences.
if _LOADED["mod_statusbar"] then return end

if not ioncore.load_module("mod_statusbar") then
    return
end

local mod_statusbar=_G["mod_statusbar"]
assert(mod_statusbar)


-- Settings {{{

-- Default settings
local defaults={
    template=string.format(
        "[ %%date || %s: %%load || %s: %%mail_new/%%mail_total ]",
        TR("load"), TR("mail")
    ),
}

local date_format_backcompat_kludge

-- }}}


-- Statusbar list {{{

local statusbars={}

local function live_statusbars()
    return coroutine.wrap(function()
                              for sb, stng in statusbars do
                                  if obj_exists(sb) then
                                      coroutine.yield(sb, stng)
                                  else
                                      statusbars[sb]=nil
                                  end
                              end
                          end)
end
     
-- }}}


-- Meter list {{{

local meters={}

--DOC
-- Inform of a value.
function mod_statusbar.inform(name, value)
    meters[name]=value
end

-- }}}


-- Template processing {{{

local function process_template(template, meter_f, text_f, stretch_f)
    local st, en, b, c, r, p
    
    while template~="" do
        -- Find '%something'
        st, en, b, r=string.find(template, '^(.-)%%(.*)')
    
        if not b then
            -- Not found
            text_f(template)
            break
        else
            if b~="" then
                -- Add preciding text as normal text element
                text_f(b)
            end
            template=r

            -- Check for '% ' and '%%'
            st, en, c, r=string.find(template, '^([ %%])(.*)')

            if c then
                if c==' ' then
                    stretch_f(c)
                else
                    text_f(c)
                end
                template=r
            else 
                -- Extract [alignment][zero padding]<meter name>
                local pat='^([<|>]?)(0*[0-9]*)([a-zA-Z0-9_]+)(.*)'
                st, en, c, p, b, r=string.find(template, pat)
                if b=="filler" then
                    stretch_f('f')
                    template=r
                elseif b then
                    meter_f(b, c, tonumber(p))
                    template=r
                end
            end
        end
    end
end



function mod_statusbar.get_template_table(stng)
    local res={}
    local m=meters --set_date(stng, meters)
    local aligns={["<"]=0, ["|"]=1, [">"]=2}
    
    process_template(stng.template,
                     -- meter
                     function(s, c, p)
                         table.insert(res, {
                             type=2,
                             meter=s,
                             align=aligns[c],
                             tmpl=meters[s.."_template"],
                             zeropad=p,
                         })
                     end,
                     -- text
                     function(t)
                         table.insert(res, {
                             type=1,
                             text=t,
                         })
                     end,
                     -- stretch
                     function(t)
                         table.insert(res, {
                             type=(t=='f' and 4 or 3),
                         })
                     end)
    return res
end

-- }}}

-- Update {{{

--DOC
-- Update statusbar contents. To be called after series
-- of \fnref{mod_statusbar.inform} calls.
function mod_statusbar.update(update_template)
    local found=false
    
    for sb, stng in live_statusbars() do
        if update_template then
            sb:set_template(mod_statusbar.get_template_table(stng))
        end
        sb:update(meters)
        found=true
    end
    
    return found
end

--DOC
-- Set the contents of a statusbar.
function mod_statusbar.set_sb(sb, stng)
    if not obj_is(sb, "WStatusBar") then
        ioncore.warn(TR("WStatusBar expected."))
        return
    end
    
    statusbars[sb] = stng
    sb:set_template(mod_statusbar.get_template_table(stng))
    sb:update(meters)
end

--DOC
-- Get the contents of a statusbar.
function mod_statusbar.get_sb(sb)
    return statusbars[sb]
end

-- }}}


-- ion-statusd support {{{

local statusd_pid=0
local statusd_cfg=" -q "
local statusd_modules={}

function mod_statusbar.rcv_statusd(str)
    local data=""
    local updatenw=false
    local updated=false

    local function doline(i)
        if i=="." then
            mod_statusbar.update(updatenw)
            updated=true
        else
            local _, _, m, v=string.find(i, "^([^:]+):%s*(.*)")
            if m and v then
                mod_statusbar.inform(m, v)
                updatenw=updatenw or string.find(m, "_template")
            end
        end
    end
    
    while str do
        updated=false
        data=string.gsub(data..str, "([^\n]*)\n", doline)
        str=coroutine.yield(updated)
    end
    
    ioncore.warn(TR("ion-statusd quit."))
    statusd_pid=0
    meters={}
    mod_statusbar.update(updatenw)
end


local function get_statusd_params()
    local mods={}
    for sb, stng in live_statusbars() do
        process_template(stng.template,
                         -- meter
                         function(s)
                             local _, _, m = string.find(s, "^([^_]+)")
                             if m then
                                 mods[m]=true
                             end
                         end,
                         -- text
                         function() 
                         end,
                         -- stretch
                         function()
                         end)
    end
    local params=statusd_cfg
    table.foreach(mods, function(k) params=params.." -m "..k end)
    return params
end


function mod_statusbar.cfg_statusd(cfg)
    if date_format_backcompat_kludge then
        if not cfg.date then
            cfg=table.copy(cfg, false)
            cfg.date={date_format=date_format_backcompat_kludge}
        elseif not cfg.date.date_format then
            cfg=table.copy(cfg, true)
            cfg.date.date_format=date_format_backcompat_kludge
        end
    end

    --TODO: don't construct file name twice.
    ioncore.write_savefile("cfg_statusd", cfg)
    return ioncore.get_savefile("cfg_statusd")
end


function mod_statusbar.rcv_statusd_err(str)
    io.stderr:write(str)
end


--DOC
-- Launch ion-statusd with configuration table \var{cfg}.
function mod_statusbar.launch_statusd(cfg)
    if statusd_pid>0 then
        return
    end
    
    local statusd=ioncore.lookup_script("ion-statusd")
    if not statusd then
        ioncore.warn(TR("Could not find %s", script))
        return
    end

    local statusd_errors
    local function initrcverr(str)
        statusd_errors=(statusd_errors or "")..str
    end

    local cfg=mod_statusbar.cfg_statusd(cfg or {})
    
    local cmd=statusd.." -c "..cfg..get_statusd_params()
    local rcv=coroutine.wrap(mod_statusbar.rcv_statusd)
    local rcverr=mod_statusbar.rcv_statusd_err

    statusd_pid=mod_statusbar.launch_statusd_(cmd, 
                                              rcv, initrcverr, 
                                              rcv, rcverr)

    if statusd_errors then
        warn(TR("Errors starting ion-statusd:\n")..statusd_errors)
    end

    if statusd_pid<=0 then
        warn(TR("Failed to start ion-statusd."))
    end                                              
end

--}}}


-- Initialisation and default settings {{{

--DOC 
-- Set defaults. For Backwards backcompat.
function mod_statusbar.set(t)
    defaults=table.join(t, defaults)
end


--DOC 
-- Get defaults. For Backwards backcompat.
function mod_statusbar.get(t)
    return defaults
end

    
--DOC
-- Create a statusbar.
function mod_statusbar.create(param_)
    if param_.date_format and not date_format_backcompat_kludge then
        date_format_backcompat_kludge=param_.date_format
    end
    
    local param=table.join(param_, defaults)
    
    local scr=ioncore.find_screen_id(param.screen or 0)
    if not scr then
        error(TR("Screen not found."))
    end
    
    if not param.force then
        local stdisp=scr:get_stdisp()
        if stdisp and stdisp.reg then
            error(TR("Screen already has an stdisp. Refusing to create a "..
                     "statusbar."))
        end
    end
    
    local sb=scr:set_stdisp({
        type="WStatusBar", 
        pos=(param.pos or "bl"),
        name="*statusbar*",
    })
    
    if not sb then
        error(TR("Failed to create statusbar."))
    end

    statusbars[sb]=param
    sb:set_template(mod_statusbar.get_template_table(param))
    sb:update(meters)
    
    return sb
end

-- }}}


-- Mark ourselves loaded.
_LOADED["mod_statusbar"]=true


-- Load user configuration file
dopath('cfg_statusbar', true)

-- Launch statusd if the user didn't launch it.
if statusd_pid<=0 then
    mod_statusbar.launch_statusd()
end
