
io.stdout:write([[
/* Automatically generated. */
                  
#include <ioncore/modules.h>

]]);

for _, v in ipairs(arg) do
    io.stdout:write(string.format([[
extern bool %s_init();
extern void %s_deinit();
]], v, v));

end

io.stdout:write([[

WStaticModuleInfo ioncore_static_modules[]={
]])

for _, v in ipairs(arg) do
    io.stdout:write(string.format(
        '    {"%s", %s_init, %s_deinit, FALSE},\n',
        v, v, v));                          
end

io.stdout:write([[
    {NULL, NULL, NULL, FALSE}
};
]])        

