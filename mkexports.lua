--
-- ion/mkexports.lua
-- 
-- Copyright (c) Tuomo Valkonen 2003.
-- 
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--
-- 
-- This is a script to automatically generate exported function registration
-- code and documentation for those from C source.
-- 
-- The script can also parse documentation comments from Lua code.
-- 

-- Helper functions {{{

function errorf(fmt, ...)
    error(string.format(fmt, unpack(arg)), 2)
end

function matcherr(s)
    error(string.format("Parse error in \"%s...\"", string.sub(s, 1, 50)), 2)
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

function add_class(cls)
    if cls~="WObj" and not classes[cls] then
        classes[cls]={}
    end
end

function sort_classes(cls)
    local sorted={}
    local inserted={}
    
    local function insert(cls)
        if classes[cls] and not inserted[cls] then
            if classes[cls].parent then
                insert(classes[cls].parent)
            end
            inserted[cls]=true
            table.insert(sorted, cls)
        end
    end
    
    for cls in classes do
        insert(cls)
    end
    
    return sorted
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
    local is_const=""
    local s, e=string.find(t, "^const +")
    if s then
        is_const="const "
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
    desc = ct2desc[is_const .. tn]
    
    if not desc or desc=="o" then
        s, e=string.find(tn, "^W[%w_]*%*$")
        if s then
            desc="o"
            otype=string.sub(tn, s, e-1)
            add_class(otype)
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
    
    local function do_do_export(cls, efn, ot, fn, param)
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
        
        if cls=="?" then
            if string.sub(idesc, 1, 1)~="o" then
                error("Invalid class for " .. fn)
            end
            cls=itypes[1]
        end
        
        -- Generate call handler name
        
        local fninfo={
            doc=doc,
            odesc=odesc,
            otype=otype,
            idesc=idesc,
            itypes=itypes,
            ivars=ivars,
            exported_name=efn,
            class=cls,
        }
        
        add_chnd(fninfo)
        add_class(cls)
        
        if not classes[cls].fns then
            classes[cls].fns={}
        end
        
        assert(not classes[cls].fns[fn], "Function " .. fn .. " multiply defined!")

        classes[cls].fns[fn]=fninfo

        -- Reset
        doc=nil
    end

    -- Handle EXTL_EXPORT otype fn(args)
    local function do_export(s)
        local pat="^[%s\n]+EXTL_EXPORT[%s\n]+([%w%s_*]+[%s\n*])([%w_]+)[%s\n]*(%b())"
        local st, en, ot, fn, param=string.find(s, pat)
        if not st then matcherr(s) end
        do_do_export("", fn, ot, fn, param)
    end

    -- Handle EXTL_EXPORT_AS(exported_fn) otype fn(args)
    local function do_export_as(s)
        local pat="^[%s\n]+EXTL_EXPORT_AS%(%s*([%w_]+)%s*%)[%s\n]+([%w%s_*]+[%s\n*])([%w_]+)[%s\n]*(%b())"
        local st, en, efn, ot, fn, param=string.find(s, pat)
        if not st then matcherr(s) end
        do_do_export("", efn, ot, fn, param)
    end

    -- Handle EXTL_EXPORT_MEMBER otype prefix_fn(class, args)
    local function do_export_member(s)
        local pat="^[%s\n]+EXTL_EXPORT_MEMBER[%s\n]+([%w%s_*]+[%s\n*])([%w_]+)[%s\n]*(%b())"
        local st, en, ot, fn, param=string.find(s, pat)
        if not st then matcherr(s) end
        local efn=string.gsub(fn, ".-_(.*)", "%1")
        do_do_export("?", efn, ot, fn, param)
    end

    -- Handle EXTL_EXPORT_MEMBER_AS(class, member_fn) otype fn(args)
    local function do_export_member_as(s)
        local pat="^[%s\n]+EXTL_EXPORT_MEMBER_AS%(%s*([%w_]+)%s*,%s*([%w_]+)%s*%)[%s\n]+([%w%s_*]+[%s\n*])([%w_]+)[%s\n]*(%b())"
        local st, en, cls, efn, ot, fn, param=string.find(s, pat)
        if not st then matcherr(s) end
        do_do_export(cls, efn, ot, fn, param)
    end
    
    local function do_implobj(s)
        local pat="^[%s\n]+IMPLOBJ%(%s*([%w_]+)%s*,%s*([%w_]+)%s*, [^)]*%)"
        local st, en, cls, par=string.find(s, pat)
        if not st then matcherr(s) end
        add_class(cls)
        classes[cls].parent=par
    end
    
    local lookfor={
        ["/%*EXTL_DOC"] = do_doc,
        ["[%s\n]EXTL_EXPORT[%s\n]"] = do_export,
        ["[%s\n]EXTL_EXPORT_AS"] = do_export_as,
        ["[%s\n]EXTL_EXPORT_MEMBER[%s\n]"] = do_export_member,
        ["[%s\n]EXTL_EXPORT_MEMBER_AS"] = do_export_member_as,
        ["[%s\n]IMPLOBJ"] = do_implobj,
    }
    
    do_parse(d, lookfor)
end

function do_parse(d, lookfor)
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


-- Parser for Lua code documentation {{{

function parse_luadoc(d)
    function do_luadoc(s_)
        local st, en, b, s=string.find(s_, "\n%-%-DOC(.-)(\n.*)")
        if string.find(b, "[^%s]") then
            errorf("Syntax error while parsing \"--DOC%s\"", b)
        end
        local doc, docl=""
        while true do
            st, en, docl=string.find(s, "^\n%s*%-%-([^\n]*\n)")
            if not st then
                break
            end
            --print(docl)
            doc=doc .. docl
            s=string.sub(s, en)
        end
        
        local fn, param
        st, en, fn, param=
        string.find(s, "^\n[%s\n]*function ([%w_:%.]+)%s*(%b())")

        if fn then
            
        else
            st, en, fn=string.find(s, "^\n[%s\n]*([%w_:%.]+)%s*=")
            if not fn then
                errorf("Syntax error while parsing \"%s\"",
                       string.sub(s, 1, 50))
            end
        end

        local cls=""

        fninfo={
            doc=doc, 
            paramstr=param,
            class=cls,
        }
        
        add_class(cls)
        if not classes[cls].fns then
            classes[cls].fns={}
        end
        classes[cls].fns[fn]=fninfo
    end
    
    do_parse(d, {["\n%-%-DOC"]=do_luadoc})
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
            fprintf(h, "    if(!chko(in, %d, &OBJDESCR(%s))) return FALSE;\n",
                    k-1, t)
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

function write_class_fns(h, cls, data)
    if cls=="" then
        fprintf(h, "\n\nstatic ExtlExportedFnSpec exports[] = {\n")
    else
        fprintf(h, "\n\nstatic ExtlExportedFnSpec %s_exports[] = {\n", cls)
    end
    
    for fn, info in data.fns do
        local ods, ids="NULL", "NULL"
        if info.odesc~="v" then
            ods='"' .. info.odesc .. '"'
        end
        
        if info.idesc~="" then
            ids='"' .. info.idesc .. '"'
        end
        
        fprintf(h, "    {\"%s\", %s, %s, %s, (ExtlL2CallHandler*)%s},\n",
                info.exported_name, fn, ids, ods, info.chnd)
    end
    
    fprintf(h, "    {NULL, NULL, NULL, NULL, NULL}\n};\n\n")
end

function write_exports(h)
    -- begin blockwrite
    h:write([[
/* Automatically generated by mkexports.lua */
#include <ioncore/common.h>
#include <ioncore/extl.h>
#include <ioncore/objp.h>

]])
    -- end blockwrite

    -- Write class infos
    for c, data in classes do
        -- WObj is defined in obj.h which we include.
        if c~="" then
            fprintf(h, "INTROBJ(%s);\n", c)
            if not data.parent and data.fns then
                error("Class functions can only be registered if the object "
                      .. "is implemented in the module in question.")
            end
        end
    end
    
    -- begin blockwrite
    h:write([[
              
static bool chko(ExtlL2Param *in, int ndx, WObjDescr *descr)
{
    WObj *o=in[ndx].o;
    if(wobj_is(o, descr)) return TRUE;
    warn("Type checking failed in level 2 call handler for parameter %d "
	 "(got %s, expected %s).", ndx, WOBJ_TYPESTR(o), descr->name);
    return FALSE;
}

]])
    -- end blockwrite
    
    -- Write L2 call handlers
    for name, info in chnds do
        writechnd(h, name, info)
    end
    
    fprintf(h, "\n")
    
    for cls, data in classes do
        if data.fns then
            -- Write function declarations
            for fn in data.fns do
                fprintf(h, "extern void %s();\n", fn)
            end
            -- Write function table
            write_class_fns(h, cls, data)
        end
    end
    
    fprintf(h, "bool %s_register_exports()\n{\n", module);

    local sorted_classes=sort_classes()
    
    for _, cls in sorted_classes do
        if cls=="" then
            fprintf(h, "    if(!extl_register_functions(exports)) return FALSE;\n");
        elseif classes[cls].fns then
            fprintf(h, "    if(!extl_register_class(\"%s\", %s_exports, \"%s\")) return FALSE;\n",
                    cls, cls, classes[cls].parent);
        elseif classes[cls].parent then
            fprintf(h, "    if(!extl_register_class(\"%s\", NULL, \"%s\")) return FALSE;\n",
                    cls, classes[cls].parent);
        end
    end

    fprintf(h, "    return TRUE;\n}\n\nvoid %s_unregister_exports()\n{\n", 
            module);
    
    for _, cls in sorted_classes do
        if cls=="" then
            fprintf(h, "    extl_unregister_functions(exports);\n");
        elseif classes[cls].fns then
            fprintf(h, "    extl_unregister_class(\"%s\", %s_exports);\n",
                    cls, cls);
        elseif classes[cls].parent then
            fprintf(h, "    extl_unregister_class(\"%s\", NULL);\n",
                    cls);
        end
    end
    
    fprintf(h, "}\n\n");
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
    if info.exported_name then
        fn=info.exported_name
    end
    
    if not lua_input then
        if info.class~="" then
            fprintf(h, "\\index{%s@\\type{%s}!", info.class, info.class)
            fprintf(h, "%s@\\code{%s}}\n", texfriendly(fn), fn)
        end
        fprintf(h, "\\index{%s@\\code{%s}}\n", texfriendly(fn), fn)
    else
        local fnx=fn
        fprintf(h, "\\index{")
        fnx=string.gsub(fnx, "(.-)%.", 
                        function(s) 
                            fprintf(h, "%s@\\type{%s}!", texfriendly(s), s)
                            return ""
                        end)
        fprintf(h, "%s@\\code{%s}}\n", texfriendly(fnx), fnx)
    end
    
    if info.class~="" then
        fprintf(h, "\\hyperlabel{fn:%s.%s}", info.class, fn)
    else
        fprintf(h, "\\hyperlabel{fn:%s}", fn)
    end
    if lua_input then
        if info.paramstr then
            fprintf(h, "\\synopsis{%s%s}\n", fn, info.paramstr)
        else
            fprintf(h, "\\funcname{%s}\n", fn)
        end
    else
        fprintf(h, "\\synopsis{%s ", tohuman(info.odesc, info.otype));
        if info.class~="" then
            fprintf(h, "%s.", info.class);
        end
        fprintf(h, "%s(", fn);
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
    end
    h:write("\\begin{funcdesc}\n" .. trim(info.doc or "").. "\n\\end{funcdesc}\n")
    fprintf(h, "\\end{function}\n\n")
end


function write_class_documentation(h, cls)
    sorted={}
    
    if not classes[cls].fns then
        return
    end
    
    if cls~="" then
        fprintf(h, "\n\n\\subsection{\\type{%s} functions}\n\n", cls);
        --[[
        fprintf(h, "\\label{sec:%s-fns}", cls);

        fprintf(h, "This section lists member functions for the class "
                .."\\type{%s}.\n", cls)
        
        if classes[cls].parent then
            fprintf(h, "this class has \\type{%s} as parent class, so see"
                    .."section \\ref{sec:%s-fns} for more functions that work"
                    .."on objects of this type.\n", classes[cls].parent,
                    classes[cls].parent);
        end
        fprintf(h, "\n");
        --]]
    end
            
    for fn in classes[cls].fns do
        table.insert(sorted, fn)
    end
    table.sort(sorted)
    
    for _, fn in ipairs(sorted) do
        write_fndoc(h, fn, classes[cls].fns[fn])
    end
end


function write_documentation(h)
    sorted={}

    for cls in classes do
        table.insert(sorted, cls)
    end
    table.sort(sorted)
    
    for _, cls in ipairs(sorted) do
        write_class_documentation(h, cls)
    end
end

-- }}}


-- main {{{

inputs={}
outh=io.stdout
make_docs=false
lua_input=false
module=""
i=1

while arg[i] do
    if arg[i]=="-help" then
        print("Usage: mkexports.lua [-mkdoc] [-help] [-o outfile] [-module module] inputs...")
        return
    elseif arg[i]=="-mkdoc" then
        make_docs=true
    elseif arg[i]=="-luadoc" then
        make_docs=true
        lua_input=true
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
    if lua_input then
        parse_luadoc("\n" .. data .. "\n")
    else
        parse("\n" .. data .. "\n")
    end
end

if make_docs then
    write_documentation(outh)
else
    write_exports(outh)
end

-- }}}

