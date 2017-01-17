-- Copyright (c) 2011, M Rawash <mrawash@gmail.com>
-- 
-- Released under the most recent GPL <http://www.gnu.org/licenses/gpl.html>
--
-- Subtle(wm)-like window management
--

function notsubtle(cf,pos)
	local cmx = assert(cf:mx_current())
	local cws = notioncore.find_manager(cf,'WGroupWS')
	local stat = 'norm'
	local m = .50

	if cf:is_grattr('norm') then
		stat = 'big'
		m = .66
	end

	if cf:is_grattr('big') then
		stat = 'small'
		m = .34
	end

	local g = {x=0,y=0,w=math.floor(cws:geom().w*m),h=math.floor(cws:geom().h*m)}

	if pos:find't' or pos:find'b' then
		g.w=math.floor(cws:geom().w*.50)
	end

	if pos:find'b' then
		g.y=cws:geom().h-g.h
	end
	
	if pos:find'r' then
		g.x=cws:geom().w-g.w
	end

	if pos=='t' or pos=='b' then
		g.w=cws:geom().w
	end

	if pos=='l' or pos=='r' then
		g.h=cws:geom().h
	end

	if pos=='c' then
		g.w=math.floor(cws:geom().w*m)
		g.h=math.floor(cws:geom().h*m)
		g.x=math.floor((cws:geom().w-g.w)/2)
		g.y=math.floor((cws:geom().h-g.h)/2)
	end

	if pos=='f' then
		g.w=cws:geom().w
		g.h=cws:geom().h
	end

	local nf
	local function chk(mf)
		if mf.__typename~='WFrame' then return true end
		
		fg=mf:geom()
		
		if fg.x==g.x and fg.y==g.y and fg.w==g.w and fg.h==g.h then
			nf=mf
			return false
		else
			return true
		end
	end

	if cws:managed_i(chk) then
		if cf:mx_count()>1 then
			nf=cws:attach_new{type='WFrame',geom=g,switchto=true}
			nf:set_mode('floating')
			nf:attach(cmx,{switchto=true})
		else
			nf=cf
			nf:rqgeom(g)
		end
	elseif nf~=cf then
		nf:attach(cmx,{switchto=true})
	end
	
	if stat=='small' then
		nf:set_grattr('norm','unset')
		nf:set_grattr('big','unset')
	else
		nf:set_grattr(stat,'set')
	end

	nf:display()
end

defbindings("WFrame.floating",{
	kpress(META.."KP_0", "notsubtle(_,'f')"),
	kpress(META.."KP_1", "notsubtle(_,'bl')"),
	kpress(META.."KP_2", "notsubtle(_,'b')"),
	kpress(META.."KP_3", "notsubtle(_,'br')"),
	kpress(META.."KP_4", "notsubtle(_,'l')"),
	kpress(META.."KP_5", "notsubtle(_,'c')"),
	kpress(META.."KP_6", "notsubtle(_,'r')"),
	kpress(META.."KP_7", "notsubtle(_,'tl')"),
	kpress(META.."KP_8", "notsubtle(_,'t')"),
	kpress(META.."KP_9", "notsubtle(_,'tr')"),
})

