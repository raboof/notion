##
## List of modules to build
##

MODULE_LIST = mod_ionws mod_floatws mod_query mod_menu \
	      mod_dock mod_sp mod_sm mod_panews de \
	      mod_mgmtmode

# Modules to -dlpreload into pwm if statically linking.

PWM_MODULE_LIST := $(MODULE_LIST)
