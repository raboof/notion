-- Ion xinerama module - lua setup
-- 
-- by Tomas Ebenlendr <ebik@ucw.cz>
--
-- This library is free software; you can redistribute it and/or
-- modify it under the terms of the GNU Lesser General Public
-- License as published by the Free Software Foundation; either
-- version 2.1 of the License,or (at your option) any later version.
--
-- This library is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
-- Lesser General Public License for more details.
--
-- You should have received a copy of the GNU Lesser General Public
-- License along with this library; if not,write to the Free Software
-- Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
--

-- This is a slight abuse of the package.loaded variable perhaps, but
-- library-like packages should handle checking if they're loaded instead of
-- confusing the user with require/include differences.
if package.loaded["mod_xinerama"] then return end

if not ioncore.load_module("mod_xinerama") then
    return
end

local mod_xinerama=_G["mod_xinerama"]

assert(mod_xinerama)

-- Helper functions {{{

-- }}}


-- Mark ourselves loaded.
package.loaded["mod_xinerama"]=true


-- Load configuration file
dopath('cfg_xinerama', true)

local screens = mod_xinerama.query_screens();
if screens then
    mod_xinerama.setup_screens_once(screens);
end 
