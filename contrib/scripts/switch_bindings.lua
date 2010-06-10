-- switch_bindings.lua
--
-- Switch between different set of keybindings. You can use it to temporarily
-- disable the keybindings by pressing META+F8. All the subsequent keystrokes
-- will be sent to the application, until you press META+F8 again, at which
-- point the previous bindings will be restored.
--
-- Author: Sadrul Habib Chowdhury (Adil) <imadil |at| gmail |dot| com>
-- Adjustments by: Canaan Hadley-Voth
--

--
-- second set of bindings
--
local alternate = {
	WScreen = {
		kpress(META.."F8", "toggle_bindings()"),
	}
}

function toggle_bindings()
	local save = table.copy(ioncore.getbindings(), true)
	local kcb_area

	-- First empty all the bindings
	for name, bindings in pairs(save) do
		for index, bind in pairs(bindings) do
			if bind.area then
			    kcb_area = bind.kcb.."@"..bind.area
			else
			    kcb_area = bind.kcb
			end
			-- This can most definitely be improved
			if bind.action == "kpress" then
				ioncore.defbindings(name, {kpress(bind.kcb, nil)})
			-- "kpress_wairel" appearing as bind.action
			elseif string.find(bind.action, "kpress_wai") then
				ioncore.defbindings(name, {kpress_wait(bind.kcb, nil)})
			elseif bind.action == "mpress" then
				ioncore.defbindings(name, {mpress(kcb_area, nil)})
			elseif bind.action == "mdrag" then
				ioncore.defbindings(name, {mdrag(kcb_area, nil)})
			elseif bind.action == "mclick" then
				ioncore.defbindings(name, {mclick(kcb_area, nil)})
			elseif bind.action == "mdblclick" then
				ioncore.defbindings(name, {mdblclick(kcb_area, nil)})
			end
		end
	end

	-- Store the last saved on
	for a, b in pairs(alternate) do
		ioncore.defbindings(a, b)
	end

	alternate = save
end

ioncore.defbindings("WScreen", {
	kpress(META.."F8", "toggle_bindings()"),
})
