--
-- A script to automatically generate exported function registration
-- code and documentation for those from C source.
-- 


-- Helper functions {{{

function errorf(fmt, ...)
    error(string.format(fmt, unpack(arg)), 2)
end

function fprintf(h, fmt, ...)
    h:write(string.format(fmt, unpack(arg)))
end

function trim(str)
    return string.gsub(str, "^[%s\n]*(.-)[%s\n]*$", "%1")
end

-- }}}


-- Some conversion tables {{{

desc2ct={
    ["v"]="void",
    ["i"]="int",
    ["d"]="double",
    ["b"]="bool",
    ["t"]="ExtlTab",
    ["f"]="ExtlFn",
    ["o"]="WObj*",
    ["s"]="char*",
    ["S"]="const char*",
}

ct2desc={
    ["uint"] = "i",
}

for d, t in desc2ct do
    ct2desc[t]=d
end

desc2human={
    ["v"]="void",
    ["i"]="integer",
    ["d"]="double",
    ["b"]="bool",
    ["t"]="table",
    ["f"]="function",
    ["o"]="object",
    ["s"]="string",
    ["S"]="string",
}

-- }}}


-- Parser {{{

local fns={}
local classes={}
local chnds={}

function add_chnd(fnt)
    local odesc=string.gsub(fnt.odesc, "S", "s")
    local idesc=string.gsub(fnt.idesc, "S", "s")
    local str="l2chnd_" .. odesc .. "_" .. idesc .. "_"
    
    for i, t in ipairs(fnt.itypes) do
        str=str .. "_" .. t
    end
    
    chnds[str]={odesc=odesc, idesc=idesc, itypes=fnt.itypes}
    fnt.chnd=str
end

function parse_type(t)
    local desc, otype, varname="?", "", ""
    
    -- Remove whitespace at start end end of the string and compress elsewhere.
    t=string.gsub(trim(t), "[%s\n]+", " ")
    -- Remove spaces around asterisks.
    t=string.gsub(t, " *%* *", "*")
    -- Add space after asterisks.
    t=string.gsub(t, "%*", "* ")
    
    -- Check for const
    local is_const=false
    local s, e=string.find(t, "^const +")
    if s then
        is_const=true
        t=string.sub(t, e+1)
    end
    
    -- Find variable name part
    tn=t
    s, e=string.find(tn, " ")
    if s then
        varname=string.sub(tn, e+1)
        tn=string.sub(tn, 1, s-1)
        assert(not string.find(varname, " "))
    end
    
    -- Try to check for supported types
    if ct2desc[tn] then
        desc=ct2desc[tn]
    end
    
    if desc=="o" or desc=="?" then
        s, e=string.find(tn, "^W[%w_]*%*$")
        if s then
            desc="o"
            otype=string.sub(tn, s, e-1)
            classes[otype]=true
        else
            errorf("Error parsing type from \"%s\"", t)
        end
    end
    
    return desc, otype, varname
end

function parse(d)
    local doc=nil
    -- Handle /*EXTL_DOC ... */
    local function do_doc(s)
        --s=string.gsub(s, "/%*EXTL_DOC(.-)%*/", "%1")
        local st=string.len("/*EXTL_DOC")
        local en, _=string.find(s, "%*/")
        if not en then
            errorf("Could not find end of comment in \"%s...\"",
                   string.sub(s, 1, 50))
        end
        
        s=string.sub(s, st+1, en-1)
        s=string.gsub(s, "\n[%s]*%*", "\n")
        doc=s
    end
    
    -- Handle EXTL_EXPORT otype fn(args)
    local function do_export(s)
        local pat="^[%s\n]+EXTL_EXPORT[%s\n]+([%w%s_*]+[%s\n*])([%w_]+)[%s\n]*(%b())"
        local st, en, ot, fn, param=string.find(s, pat)
        if not st then
            errorf("Regex matching error in \"%s...\"", string.sub(s, 1, 50))
        end
        
        local odesc, otype=parse_type(ot)
        local idesc, itypes, ivars="", {}, {}
        
        -- Parse arguments
        param=string.sub(param, 2, -2)
        if string.find(param, "[()]") then
            errorf("Error: parameters to %s contain parantheses", fn)
        end
        param=trim(param)
        if string.len(param)>0 then
            for p in string.gfind(param .. ",", "([^,]*),") do
                local spec, objtype, varname=parse_type(p)
                idesc=idesc .. spec
                table.insert(itypes, objtype)
                table.insert(ivars, varname)
            end
        end
        
        -- Generate call handler name
        
        assert(not fns[fn], "Function " .. fn .. " multiply defined!")
        
        fns[fn]={
            doc=doc,
            odesc=odesc,
            otype=otype,
            idesc=idesc,
            itypes=itypes,
            ivars=ivars,
        }
        add_chnd(fns[fn])
        
        -- Reset
        doc=nil
    end
    
    local lookfor={
        ["/%*EXTL_DOC"] = do_doc,
        ["[%s\n]EXTL_EXPORT"] = do_export,
    }
    
    while true do
        local mins, mine, minfn=string.len(d)+1, nil, nil
        for str, fn in pairs(lookfor) do
            local s, e=string.find(d, str)
            if s and s<mins then
                mins, mine, minfn=s, e, fn
            end
        end
        
        if not minfn then
            return
        end
        
        minfn(string.sub(d, mins))
        d=string.sub(d, mine+1)
    end
end

-- }}}


-- Export output {{{

function writechnd(h, name, info)
    local oct=desc2ct[info.odesc]
        
    -- begin blockwrite
    fprintf(h, [[
static bool %s(%s (*fn)(), ExtlL2Param *in, ExtlL2Param *out)
{
]], name, oct)
    -- end blockwrite

    -- Generate type checking code
    for k, t in info.itypes do
        if t~="" then
            fprintf(h, "    if(!WOBJ_IS(in[%d].o, %s)){ return fail(%d, in[%d].o, \"%s\"); }\n",
                    k-1, t, k-1, k-1, t)
        end
    end

    -- Generate function call code
    if info.odesc=="v" then
        fprintf(h, "    fn(")
    else
	fprintf(h, "    out[0].%s=fn(", info.odesc)
    end
    
    comma=""
    for k=1, string.len(info.idesc) do
        fprintf(h, comma .. "in[%d].%s", k-1, string.sub(info.idesc, k, k))
	comma=", "
    end
    fprintf(h, ");\n    return TRUE;\n}\n")
end    

function write_exports(h)
    -- begin blockwrite
    h:write([[
/* Automatically generated by mkexports.pl */
#include <ioncore/common.h>
#include <ioncore/extl.h>
#include <ioncore/objp.h>

static bool fail(int ndx, WObj *o, const char *s)
{
    warn("Type checking failed in level 2 call handler for parameter %d "
	 "(got %s, expected %s).", ndx, WOBJ_TYPESTR(o), s);
    return FALSE;
}

]])
    -- end blockwrite
    
    -- Write class infos
    for c in classes do
        fprintf(h, "INTROBJ(%s);\n", c)
    end
    
    fprintf(h, "\n")
    
    -- Write L2 call handlers
    for name, info in chnds do
        writechnd(h, name, info)
    end
    
    fprintf(h, "\n")
    
    -- Write function declarations
    for fn in fns do
        fprintf(h, "extern void %s();\n", fn)
    end
    
    -- Write function table
    fprintf(h, "\n\nstatic ExtlExportedFnSpec %s_exports[] = {\n", module or "")
    
    for fn, info in fns do
        local ods, ids="NULL", "NULL"
        if info.odesc~="v" then
            ods='"' .. info.odesc .. '"'
        end
        
        if info.idesc~="" then
            ids='"' .. info.idesc .. '"'
        end
        
        fprintf(h, "    {\"%s\", %s, %s, %s, (ExtlL2CallHandler*)%s},\n",
                fn, fn, ids, ods, info.chnd)
    end
    
    fprintf(h, "    {NULL, NULL, NULL, NULL, NULL}\n};\n\n")
    
    local str=string.gsub([[
bool module_register_exports()
{
    int i;
    for(i=0; module_exports[i].fn; i++){
        extl_register_function(module_exports+i);
    }
    return TRUE;
}

void module_unregister_exports()
{
    int i;
    for(i=0; module_exports[i].fn; i++){
        extl_unregister_function(module_exports+i);
    }
}
]], "module", module)
    h:write(str)
end

-- }}}


-- Documentation output {{{

function tohuman(desc, objtype)
    if objtype~="" then
        return objtype
    else 
        return desc2human[desc]
    end
end

function texfriendly(name)
    return string.gsub(name, "_", "-")
end

function write_fndoc(h, fn, info)
    fprintf(h, "\\begin{function}\n")
    fprintf(h, "\\index{%s@\\code{%s}}\n", texfriendly(fn), fn)
    fprintf(h, "\\hyperlabel{fn:%s}", fn)
    fprintf(h, "\\synopsis{%s %s(", tohuman(info.odesc, info.otype), fn)
    local comma=""
    for i, varname in info.ivars do
        fprintf(h, comma .. "%s", tohuman(string.sub(info.idesc, i, i),
                                          info.itypes[i]))
        if varname then
            fprintf(h, " %s", varname)
        end
        comma=", "
    end
    fprintf(h, ")}\n")
    h:write("\\begin{funcdesc}\n" .. trim(info.doc).. "\n\\end{funcdesc}\n")
    fprintf(h, "\\end{function}\n\n")
end

function write_documentation(h)
    sorted={}
    
    for fn in fns do
        table.insert(sorted, fn)
    end
    table.sort(sorted)
    
    for _, fn in ipairs(sorted) do
        if fns[fn].doc then
            write_fndoc(h, fn, fns[fn])
        end
    end
end

-- }}}


-- main {{{

inputs={}
outh=io.stdout
make_docs=false
module=""
i=1

while arg[i] do
    if arg[i]=="-help" then
        print("Usage: mkexports.lua [-mkdoc] [-help] [-o outfile] [-module module] inputs...")
        return
    elseif arg[i]=="-mkdoc" then
        make_docs=true
    elseif arg[i]=="-o" then
        i=i+1
        outh, err=io.open(arg[i], "w")
        if not outh then
            errorf("Could not open %s: %s", arg[i], err)
        end
    elseif arg[i]=="-module" then
        i=i+1
        module=arg[i]
        if not module then
            error("No module given")
        end
    else
        table.insert(inputs, arg[i])
    end
    i=i+1
end

if table.getn(inputs)==0 then
    error("No inputs")
end

for _, ifnam in inputs do
    h, err=io.open(ifnam, "r")
    if not h then
            errorf("Could not open %s: %s", ifnam, err)
    end
    print("Scanning " .. ifnam .. " for exports.")
    data=h:read("*a")
    h:close()
    parse("\n" .. data .. "\n")
end

if make_docs then
    write_documentation(outh)
else
    write_exports(outh)
end

-- }}}

