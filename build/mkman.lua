
-- Translations {{{

local translations={}

local function gettext(x)
    local t=translations[x]
    if not t or t=="" then
        return x
    else
        return t
    end
end

local function TR(x, ...)
    return string.format(gettext(x), unpack(arg))
end

local function read_translations(pofile)
    local f, err=io.open(pofile)
    if not f then 
        error(err) 
    end
    
    local msgid, msgstr, st, en
    
    for l in f:lines() do
        if string.find(l, "^msgid") then
            if msgid then
                assert(msgstr)
                translations[msgid]=msgstr
                msgstr=nil
            end
            st, en, msgid=string.find(l, '^msgid%s*"(.*)"%s*$')
        elseif string.find(l, "^msgstr") then
            assert(msgid and not msgstr)
            st, en, msgstr=string.find(l, '^msgstr%s*"(.*)"%s*$')
        elseif not (string.find(l, "^%s*#") or string.find(l, "^%s*$")) then
            local st, en, str=string.find(l, '^%s*"(.*)"%s*$')
            assert(msgid or msgstr)
            if not msgstr then
                msgid=msgid..str
            else
                msgstr=msgstr..str
            end
        end
    end

    if msgid then
        assert(msgstr)
        translations[msgid]=msgstr
    end
    
    f:close()
end

-- }}}


-- File parsing {{{

local function dobindings(fn, bindings)
    local p={}
    
    p.MOD1="Mod1+"
    p.MOD2=""
    
    function p.bdoc(text)
        return {action = "doc", text = text}
    end
    
    function p.submap(kcb_, list)
        if not list then
            return function(lst)
                       return submap(kcb_, lst)
                   end
        end
        return {action = "kpress", kcb = kcb_, submap = list}
    end
    
    local function putcmd(cmd, guard, t)
        t.cmd=cmd
        t.guard=guard
        return t
    end
    
    function p.kpress(keyspec, cmd, guard)
        return putcmd(cmd, guard, {action = "kpress", kcb = keyspec})
    end
    
    function p.kpress_wait(keyspec, cmd, guard)
        return putcmd(cmd, guard, {action = "kpress_waitrel", kcb = keyspec})
    end
    
    local function mact(act_, kcb_, cmd, guard)
        local st, en, kcb2_, area_=string.find(kcb_, "([^@]*)@(.*)")
        return putcmd(cmd, guard, {
            action = act_,
            kcb = (kcb2_ or kcb_),
            area = area_,
        })
    end
    
    function p.mclick(buttonspec, cmd, guard)
        return mact("mclick", buttonspec, cmd, guard)
    end
    
    function p.mdblclick(buttonspec, cmd, guard)
        return mact("mdblclick", buttonspec, cmd, guard)
    end
    
    function p.mpress(buttonspec, cmd, guard)
        return mact("mpress", buttonspec, cmd, guard)
    end

    function p.mdrag(buttonspec, cmd, guard)
        return mact("mdrag", buttonspec, cmd, guard)
    end
    
    function p.defbindings(context, bnd)
        assert(not bindings[context])
        bindings[context]=bnd
    end
    
    local env=setmetatable({}, {
        __index=p, 
        __newindex=function(x) 
                       error("Setting global "..tostring(x))
                   end,
    })
    setfenv(fn, env)
    fn()
    return bindings
end

local function parsefile(f, bindings)
    local fn, err=loadfile(f)
    if not fn then
        error(err)
    end
    
    return dobindings(fn, bindings)
end
    
-- }}}


-- Binding output {{{

local function docgroup_bindings(bindings)
    local out={}
    local outi=0
    
    local function parsetable(t, prefix)
        for _, v in ipairs(t) do
            if v.kcb then
                v.kcb=string.gsub(v.kcb, "AnyModifier%+", "")
            end
            if v.action=="doc" then
                outi=outi+1
                out[outi]={doc=v.text, bindings={}}
            elseif v.submap then
                parsetable(v.submap, prefix..v.kcb.." ")
            else
                assert(out[outi])
                v.kcb=prefix..v.kcb
                table.insert(out[outi].bindings, v)
            end
        end
    end
    
    parsetable(bindings, "")
    
    return out
end

local nact={
    ["mpress"]=TR("press"),
    ["mclick"]=TR("click"),
    ["mdrag"]=TR("drag"),
    ["mdblclick"]=TR("double click"),
}

local function combine_bindings(v)    
    local first=true
    local s=""
    for _, b in ipairs(v.bindings) do
        if not first then
            s=s..', '
        end
        first=false
        if b.action=="kpress" or b.action=="kpress_waitrel" then
            s=s..b.kcb
        else
            if not b.area then
                s=s..TR("%s %s", b.kcb, nact[b.action])
            else
                s=s..TR("%s %s at %s", b.kcb, nact[b.action], b.area)
            end
        end
    end
    
    return s
end    

local function write_bindings_man(db)
    local function write_binding_man(v)
        return '.TP\n.B '..combine_bindings(v)..'\n'..gettext(v.doc)..'\n'
    end
    
    local s=""
    
    for _, v in ipairs(db) do
        if table.getn(v.bindings)>0 then
            s=s..write_binding_man(v)
        end
    end
    
    return s
end

-- }}}

-- Main {{{

local infile
local outfile
local bindings={}
local replaces={}

local function doargs(a)
    local i=1
    while i<=table.getn(a) do
        if a[i]=='-o' then
            outfile=a[i+1]
            i=i+2
        elseif a[i]=='-i' then
            infile=a[i+1]
            i=i+2
        elseif a[i]=='-D' then
            replaces[a[i+1]]=a[i+2]
            i=i+3
        elseif a[i]=='-po' then
            read_translations(a[i+1])
            i=i+2
        else
            parsefile(a[i], bindings)
            i=i+1
        end
    end
end

doargs(arg)

local f, err=io.open(infile)
if not f then
    error(err)
end

local of, oerr=io.open(outfile, 'w+')
if not of then
    error(oerr)
end

for l in f:lines() do
    l=string.gsub(l, '%s*BINDINGS:(%w+)%s*', 
                  function(s)
                      if not bindings[s] then
                          error('No bindings for '..s)
                      end
                      local db=docgroup_bindings(bindings[s])
                      return write_bindings_man(db)
                  end)
    
    for pat, rep in replaces do
        l=string.gsub(l, pat, rep)
    end
    
    of:write(l..'\n')
end

-- }}}

