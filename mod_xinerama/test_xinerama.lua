-- {{{ Table printing

function table_tostring (tt)
  if type(tt) == "table" then
    local sb = {}
    local first = true
    table.insert(sb, "{ ");
    for key, value in pairs (tt) do
      if first then first = false else table.insert(sb, ", ") end
      if "number" == type (key) then
	  table.insert(sb, table_tostring (value))
      else
	  table.insert(sb, key)
	  table.insert(sb, "=")
	  table.insert(sb, table_tostring (value))
      end
    end
    table.insert(sb, " }");
    return table.concat(sb)
  elseif type (tt) == "number" then
    return tostring(tt)
  else
    return '"' .. tostring(tt) .. '"'
  end
end

function print_tables(str,t)
    print(str .. ":")
    for key, value in ipairs(t) do
      print("   " .. key .. ": " .. table_tostring(value))
    end
end

-- }}}

-- {{{ mock notion contest
ioncore = { 
  load_module = function() 
    return 1
  end
}

function dopath() 
end

_G["mod_xinerama"] = {
  query_screens = function() end
}

-- }}}

-- load xinerama module lua code
dofile('mod_xinerama.lua')

function do_test(fn_name, outn, input)
    print("\nTesting:" .. fn_name)
    print_tables("Input", input)
    local output = mod_xinerama[fn_name](input)
    print_tables("Output", output)
    assert(table.getn(output, outn))
end

-- now perform some tests:

-- {{{ merge_contained_screens

do_test("merge_contained_screens",1,{
    {x=0,y=0,w=800,h=600}
})

do_test("merge_contained_screens",1,{
    {x=0,y=0,w=800,h=600},
    {x=0,y=0,w=800,h=600},
})

do_test("merge_contained_screens",1,{
    {x=0,y=0,w=800,h=600},
    {x=100,y=100,w=600,h=400},
})

do_test("merge_contained_screens",2,{
    {x=0,y=0,w=800,h=600},
    {x=100,y=100,w=800,h=600},
})

-- }}}

-- {{{ merge_overlapping_screens

do_test("merge_overlapping_screens",1,{
    {x=0,y=0,w=800,h=600}
})

do_test("merge_overlapping_screens",1,{
    {x=0,y=0,w=800,h=600},
    {x=0,y=0,w=800,h=600},
})

do_test("merge_overlapping_screens",1,{
    {x=0,y=0,w=800,h=600},
    {x=100,y=100,w=600,h=400},
})

do_test("merge_overlapping_screens",1,{
    {x=0,y=0,w=800,h=600},
    {x=100,y=100,w=800,h=600},
})


--[[
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
0 ____________________
1 |         |    |   |
2 |         |    |   |
3 |         ~~~~~|~~~~    
4 |       _______|________
5 |       |      |       |
6 ~~~~~~~~~~~~~~~~       |
7         |              |
8         |              |
9         |              |
0         ~~~~~~~~~~~~~~~~
]]--
do_test("merge_overlapping_screens",1,{
    {x=0,y=0,w=800,h=600},
    {x=400,y=400,w=800,h=600},
    {x=500,y=0,w=500,h=300}
})


--[[
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
0 ________________________
1 |              ||      |
2 |              ||      |
3 |              ||      |
4 |       _______|========
5 |       |      |       |
6 ~~~~~~~~~~~~~~~~       |
7         |              |
8         |              |
9         |              |
0         ~~~~~~~~~~~~~~~~
]]--
do_test("merge_overlapping_screens",1,{
    {x=0,y=0,w=800,h=600},
    {x=400,y=400,w=800,h=600},
    {x=800,y=0,w=400,h=400}
})
