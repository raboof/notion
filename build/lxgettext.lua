--
-- lxgettext.lua
-- 
-- Very simple 'xgettext' like tool for extracting translatable strings 
-- from Lua source code.
-- 
-- Copyright (c) Tuomo Valkonen 2004.
-- 
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--
-- 
-- To use this parser, you will need toget and build the 'ltokens' Lua
-- tokenistion library from http://www.tecgraf.puc-rio.br/~lhf/ftp/lua/.
-- Then set the LUA_PATH and LUA_SOPATH point so that the library can
-- be found. For example,
--   LUA_PATH=~/src/tokens/?.lua
--   LUA_SOPATH=~/src/tokens/
--   export LUA_PATH LUA_SOPATH
-- 

require('tokens')

local addto=nil
local keyword="TR"
local messages={}
local outfile=nil

local function try_keyword(f, tokens, i)
    local needclose=false
    local n=1
    local parts={}
    
    if i>1 then
        if tokens[i-1].token=='.' or tokens[i-1]==':' then
            -- Not a global symbol
            return n
        end
    end
    
    if not tokens[i+n] then
        return n
    end

    if tokens[i+n].token=="(" then
        needclose=true
        n=n+1
    end
    
    while true do
        if not tokens[i+n] or tokens[i+n].token~="string" then
            return n
        end

        table.insert(parts, tokens[i+n].value)
        n=n+1
        if not tokens[i+n] then
            return n
        end
        if tokens[i+n].token~=".." then
            if tokens[i+n].token==")" then
                neeclose=false
            end
            if tokens[i+n].token=="," then
                if not needclose then
                    return n
                end
                needclose=false
            end
            break
        end
        n=n+1
    end

    if neeclose then
        return n
    end
    
    local total=""
    for _, v in ipairs(parts) do
        total=total..v
    end
    
    if not messages[total] then
        messages[total]={parts=parts, where={}}
    end
    
    table.insert(messages[total].where, {file=f, line=tokens[i].line})
end


local function parsefile(f)
    local tokens, err=assert(ftokens(f))
    if not tokens then
        error(err)
    end
    local i=1
    while i<=table.getn(tokens) do
        if tokens[i].token=="name" and tokens[i].value==keyword then
            i=i+(try_keyword(f, tokens, i) or 1)
        else
            i=i+1
        end
    end
end


local function parsefiles(fname)
    local f, err=io.open(fname)
    if not f then
        error(err)
    end
    for l in f:lines() do
        if not string.find(l, '^%s*$') then
            parsefile(l)
        end
    end
end


local function eliminate()
    local msgid=nil
    
    if not addto then
        return
    end
    local f, err=io.open(addto)
    if not f then 
        error(err) 
    end
    
    local function do_eliminate(str)
        if str and messages[str] then
            messages[str]=nil
        end
    end
    
    for l in f:lines() do
        if string.find(l, "^msgid") then
            do_eliminate(msgid)
            local st, en 
            st, en, msgid=string.find(l, '^msgid%s*"(.*)"%s*$')
            assert(msgid)
        elseif string.find(l, "^msgstr") then
            do_eliminate(msgid)
            msgid=nil
        elseif not (string.find(l, "^%s*#") or string.find(l, "^%s*$")) then
            local st, en, str=string.find(l, '^%s*"(.*)"%s*$')
            assert(str)
            if msgid then
                msgid=msgid..str
            end
        end
        io.write(l.."\n")
    end
    
    f:close()
end

local function output()
    local function convert(s)
        return string.gsub(s, '[\\%c"]', 
                           function(c)
                               if c=='\n' then
                                   return '\\n'
                               elseif c=='\\' then
                                   return '\\\\'
                               elseif c=='"' then
                                   return '\\\"'
                               else
                                   error('Control characters not supported.')
                               end
                           end)
    end
                           
    for k, v in messages do
        io.write('\n#: ')
        for _, w in ipairs(v.where) do
            io.write(w.file..":"..w.line.." ")
        end
        io.write('\nmsgid ')
        for _, s in ipairs(v.parts) do
            io.write('"'..convert(s)..'"\n')
        end
        io.write('msgstr ""\n')
    end
end

local function help()
    print('Usage: lxgettext [-add source_pot] [-keyword fn] [lua files...]')
    os.exit()
end

local function doargs(a)
    local i=1
    while i<=table.getn(a) do
        if a[i]=='-f' then
            parsefiles(a[i+1])
            i=i+2
        elseif a[i]=='-out' then
            outfile=a[i+1]
            i=i+2
        elseif a[i]=='-help' then
            help()
        elseif a[i]=='-keyword' then
            keyword=a[i+1]
            i=i+2
        elseif a[i]=='-add' then
            assert(not addto)
            addto=a[i+1]
            i=i+2
        else
            parsefile(a[i])
            i=i+1
        end
    end
end

if table.getn(arg)==0 then
    help()
end

doargs(arg)

if outfile then
    local f, err=io.open(outfile, 'w+')
    if not f then
        error(err)
    end
    io.output(f)
end

eliminate()
output()
