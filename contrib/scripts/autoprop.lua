-- Automatically create a winprop for the WClientWin given.
-- Basic functionality by pinko
-- Extended by Etan Reisner <deryni@gmail.com>

-- kpress(META.."W", "autoprop(_sub, _, false)", "_sub:WClientWin")
-- kpress(MOD4.."W", "autoprop(_sub, _, true)", "_sub:WClientWin")

-- Use autoprop(_sub, nil, X) to remove a winprop

local savefile="autoprops"
local autoprops={}

function autoprop(cwin, dest, save)
  local p={}

  if type(dest)=="nil" then
    name=nil
  else
    name=dest:name()
  end

  p.class="*"
  p.role="*"
  p.instance="*"
  p.target=name

  p=table.join(cwin:get_ident(), p)
  defwinprop{
    class=p.class,
    instance=p.instance,
    role=p.role,
    target=name
  }

  if save and p.target then
    autoprops[p.class..p.role..p.instance]=p
  end

  if p.target==nil then
    autoprops[p.class..p.role..p.instance]=nil
  end
end

local function save_autoprops()
  ioncore.write_savefile(savefile, autoprops)
end

local function load_autoprops()
  local d=ioncore.read_savefile(savefile)
  if not d then
    return
  end

  table.foreach(d, function(k, tab)
                    defwinprop{
                      class=tab.class,
                      instance=tab.instance,
                      role=tab.role,
                      target=tab.target
                    }
                    autoprops[tab.class..tab.role..tab.instance]=tab
                   end)
end

local function init()
  load_autoprops()
  ioncore.get_hook("ioncore_snapshot_hook"):add(save_autoprops)
end

init()
