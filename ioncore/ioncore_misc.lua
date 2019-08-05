--
-- ion/share/ioncore_misc.lua
--
-- Copyright (c) Tuomo Valkonen 2004-2007.
--
-- See the included file LICENSE for details.
--

local group_tmpl = { type="WGroupWS" }

local default_tmpl = { switchto=true }

local empty = { type="WGroupWS", managed={} }

local layouts={
    empty = empty,
    default = empty,
}

--DOC
-- Define a new workspace layout with name \var{name}, and
-- attach/creation parameters given in \var{tab}. The layout
-- "empty" may not be defined.
function ioncore.deflayout(name, tab)
    assert(name ~= "empty")

    if name=="default" and not tab then
        layouts[name] = empty
    else
        layouts[name] = table.join(tab, group_tmpl)
    end
end

--DOC
-- Get named layout (or all of the latter parameter is set,
-- but this is for internal use only).
function ioncore.getlayout(name, all)
    if all then
        return layouts
    else
        return layouts[name]
    end
end

ioncore.set{_get_layout=ioncore.getlayout}

--DOC
-- Create new workspace on screen \var{scr}. The table \var{tmpl}
-- may be used to override parts of the layout named with \code{layout}.
-- If no \var{layout} is given, "default" is used.
function ioncore.create_ws(scr, tmpl, layout)
    local lo=ioncore.getlayout(layout or "default")

    assert(lo, TR("Unknown layout"))

    return scr:attach_new(table.join(tmpl or default_tmpl, lo))
end


--DOC
-- Find an object with type name \var{t} managing \var{obj} or one of
-- its managers.
function ioncore.find_manager(obj, t)
    while obj~=nil do
        if obj_is(obj, t) then
            return obj
        end
        obj=obj:manager()
    end
end


--DOC
-- gettext+string.format
function ioncore.TR(s, ...)
    return string.format(ioncore.gettext(s), ...)
end


--DOC
-- Attach tagged regions to \var{reg}. The method of attach
-- depends on the types of attached regions and whether \var{reg}
-- implements \code{attach_framed} and \code{attach}. If \var{param}
-- is not set, the default of \verb!{switchto=true}! is used.
function ioncore.tagged_attach(reg, param)
    if not param then
        param={switchto=true}
    end
    local tagged=function() return ioncore.tagged_first(true) end
    for r in tagged do
        local fn=((not obj_is(r, "WWindow") and reg.attach_framed)
                  or reg.attach)

        if not (fn and fn(reg, r, param)) then
            return false
        end
    end
    return true
end

function ioncore.show_live_docs(frame)
    local filename = os.tmpname()
    local keysfile = io.open(filename, "w")

    keysfile:write([[
<html>
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />

    <title>Notion keyboard reference</title>
    <style>
* {
}

body {
    font: 71%/1.5 Verdana, Sans-Serif;
    background: #eee;
}
#container {
    margin: 0;
    padding: 0;
    margin: auto;
    width: 688px;
}
#write {
    margin: 0 0 5px;
    padding: 5px;
    width: 671px;
    height: 200px;
    font: 1em/1.5 Verdana, Sans-Serif;
    background: #fff;
    border: 1px solid #f9f9f9;
    -moz-border-radius: 5px;
    -webkit-border-radius: 5px;
}
#keyboard {
    margin: 0;
    padding: 0;
    list-style: none;
}
        #keyboard li {
        float: left;
        margin: 0 5px 5px 0;
        width: 40px;
        height: 60px;
        #line-height: 40px;
        text-align: center;
        background: #fff;
        border: 1px solid #f9f9f9;
        -moz-border-radius: 5px;
        -webkit-border-radius: 5px;
        }
            .capslock, .tab, .left-shift {
            clear: left;
            }
                #keyboard .tab, #keyboard .delete, #keyboard .backspace {
                width: 70px;
                }
                #keyboard .capslock {
                width: 80px;
                }
                #keyboard .return {
                width: 77px;
                }
                #keyboard .left-shift {
                width: 95px;
                }
                #keyboard .right-shift {
                width: 109px;
                }
            #keyboard .lastitem {
            margin-right: 0;
            }
            .uppercase {
            text-transform: uppercase;
            }
            #keyboard .space {
            clear: left;
            width: 681px;
            }
            .on {
            display: none;
            }
            #keyboard li:hover {
            position: relative;
            top: 1px;
            left: 1px;
            border-color: #e5e5e5;
            cursor: pointer;
            }
.modk, .shift
{
  background-color: #CCCCCC;
}
h2
{
  margin-top: 10px;
}
.legend
{
  clear: both;
  margin: 10px;
}
td
{
  font-size: 10px;
}
.keytable
{
  margin: 0px auto;
}

#keyboard li.spacer {
  background: none;
  border: 1px solid transparent;
  width: 4px
}
</style>
  <script type="text/javascript" src="http://ajax.googleapis.com/ajax/libs/jquery/1.3.2/jquery.min.js"></script>
<script>
$().ready(function(){
  $('.shortcut').mouseenter(function() {
      $(this).css({'background-color': 'lightblue'});
      $($(this).attr('ref')).css({'background-color': 'lightblue'});
  });
  $('.shortcut').mouseleave(function() {
      $(this).css({'background-color': '#eee'});
      $($(this).attr('ref')).css({'background-color': 'white'});
  });
  $('.shortcut').each(function() {
      var shortcut = this;
      $($(this).attr('ref')).mouseenter(function() {
          $(shortcut).css({'background-color': 'lightblue'});
          $(this).css({'background-color': 'lightblue'});
      });
      $($(this).attr('ref')).mouseleave(function() {
          $(shortcut).css({'background-color': '#eee'});
          $(this).css({'background-color': 'white'});
      });
  });
});
</script>
</head>
<body>
<h1>Notion keyboard reference:</h1>

<!-- CSS keyboard from http://net.tutsplus.com/tutorials/javascript-ajax/creating-a-keyboard-with-css-and-jquery/ -->

]])

    local bindings_by_key = {}

    local function bindings_for(label)
        for _,binding in pairs(notioncore.do_getbindings()["W" .. label]) do
            if binding["doc"] and binding["label"] then
                bindings_by_key[binding["kcb"]] = binding["label"]
            end
        end
    end
    bindings_for("ClientWin")
    bindings_for("Screen")
    bindings_for("Frame")
    bindings_for("Frame.toplevel")
    bindings_for("GroupCW")
    bindings_for("Tiling")
    bindings_for("MPlex.toplevel")

    local function binding_label(label)
        if label:sub(1,2) == "->" then
            return "&rarr;" .. label:sub(3,#label)
        elseif label:sub(1,2) == "<-" then
            return "&larr;" .. label:sub(3,#label)
        elseif label:sub(1,1) == "^" then
            return "&uarr;" .. label:sub(2,#label)
        elseif label=="vframe" then
            return "&darr;frame"
        elseif label=="hsplit" then
            return "&harr;split"
        elseif label=="vsplit" then
            return "&varr;split"
        elseif label=="hmax" then
            return "&harr;max"
        elseif label=="vmax" then
            return "&varr;max"
        else
            return label
        end
    end

    local function print_key(label, class, name)
        if name == nil then
            name = label
        end
        keysfile:write("<li id=\"" .. name .. "\"")
        if class then
            keysfile:write(" class=\"" .. class .. "\"")
        end
        keysfile:write(">" .. label)
        if bindings_by_key["Mod4+" .. name] then
            keysfile:write("<br>" .. binding_label(bindings_by_key["Mod4+" .. name]))
        end
        if bindings_by_key["Shift+Mod4+" .. name] then
            keysfile:write('<br><span class="shift">' .. binding_label(bindings_by_key["Shift+Mod4+" .. name]) .. '</shift>')
        end
        keysfile:write("</li>\n")
    end

    bindings_by_key["Shift+Mod4+grave"] = bindings_by_key["Shift+Mod4+asciitilde"]

    keysfile:write("<div id=\"container\">\n")
    keysfile:write("<ul id=\"keyboard\">\n")

    print_key("Esc", nil, "Escape")
    keysfile:write("<li class=\"spacer\"></li>\n")
    for i=1,4 do print_key("F" .. i) end
    keysfile:write("<li class=\"spacer\"></li>\n")
    for i=5,8 do print_key("F" .. i) end
    keysfile:write("<li class=\"spacer\"></li>\n")
    for i=9,12 do print_key("F" .. i) end
    keysfile:write("<li class=\"spacer\"></li>\n")
    keysfile:write("<li class=\"spacer\"></li>\n")
    keysfile:write("<li class=\"spacer\"></li>\n")

    print_key("`", nil, "grave")
    for i=1,9 do print_key(i) end
    print_key("0")
    print_key("-")
    print_key("=", nil, "equal")
    print_key("Backspace", "backspace lastitem", "BackSpace")

    print_key("Tab", "tab")
    local keys = "QWERTYUIOP"
    for i=1,#keys do
        print_key(keys:sub(i,i))
    end
    print_key("[", nil, "bracketleft")
    print_key("]", nil, "bracketright")
    print_key("\\", "lastitem", "backslash")

    print_key("caps lock", "capslock", "Caps_Lock")
    keys = "ASDFGHJKL"
    for i=1,#keys do
        print_key(keys:sub(i,i))
    end
    print_key(";", nil, "semicolon")
    print_key("'", nil, "apostrophe")
    print_key("return", "return lastitem", "Return")

    keysfile:write("<li class=\"left-shift\">shift</li>\n")
    keys = "ZXCVBNM"
    for i=1,#keys do
        print_key(keys:sub(i,i))
    end
    print_key(",", nil, "comma")
    print_key(".", nil, "period")
    print_key("/", nil, "slash")
    keysfile:write("<li class=\"right-shift lastitem\">shift</li>\n")
    keysfile:write("</ul></div>")

    keysfile:write("<table class=\"keytable\" style=\"clear: left\">\n")
    keysfile:write("<tbody><tr>\n")
    keysfile:write("<td width=\"400px\" valign=\"top\">\n")
    keysfile:write("<h2>META key</h2>\n")
    keysfile:write("All keybindings are activated by pressing the META key, which by default is the 'windows' key.<br>\n")
    keysfile:write("The 'grey' bindings are activated by pressing ALTMETA (by default pressing Shift while holding META).<br><br>\n")

    local function list_keys_for(label, ctx)
        if label then
            keysfile:write("<tr><th colspan=\"2\">" .. label .. "</th></tr>\n")
        end
        for _,binding in pairs(notioncore.do_getbindings()[ctx]) do
            if binding["doc"] then
                local ref = binding["kcb"]
                if ref:sub(1,6) == "Shift+" then
                    ref = ref:sub(7, #ref)
                end
                if ref:sub(1,5) == "Mod4+" then
                    ref = ref:sub(6, #ref)
                end
                keysfile:write("<tr class=\"shortcut\" ref=\"#" .. ref .. "\"><td>" .. binding["kcb"] .. "</td><td>" .. binding["doc"] .. "</td></tr>\n")
                --if binding["label"] then
                --    bindings_by_key[binding["kcb"]] = binding["label"]
                --end
            end
        end
    end

    keysfile:write("<table>\n")
    list_keys_for("Screen", "WScreen")
    list_keys_for("Client windows", "WClientWin")
    list_keys_for(nil, "WGroupCW")
    keysfile:write("</table>\n")
    keysfile:write("</td><td valign=\"top\">")
    keysfile:write("<table>\n")
    list_keys_for("Frames", "WFrame")
    list_keys_for(nil, "WFrame.toplevel")
    list_keys_for("Tilings", "WTiling")
    list_keys_for("Tabs", "WMPlex.toplevel")
    keysfile:write("</table>\n")

    keysfile:write("</td>")
    keysfile:write("</tr></tbody>\n")
    keysfile:write("</table>\n")

    keysfile:write("</body>")
    keysfile:write("</html>")

    io.close(keysfile)

    mod_query.exec_on_merr(frame, "run-mailcap --mode=view " .. filename)
end
