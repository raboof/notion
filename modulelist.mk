##
## List of modules to build
##

MODULE_LIST = ionws floatws query de menu dock mod_sp

# Modules to -dlpreload into pwm if statically linking.

PWM_MODULE_LIST := $(MODULE_LIST)
