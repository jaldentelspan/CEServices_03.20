#
# tsd_time_input_selector_driver.tcl
#

# Create a new driver
create_driver tsd_time_input_selector_driver

# Associate it with some hardware known as "time_selector_top"
set_sw_property hw_class_name time_selector_top

# The version of this driver
set_sw_property version 1.0

# This driver may be incompatible with versions of hardware less
# than specified below. Updates to hardware and device drivers
# rendering the driver incompatible with older versions of
# hardware are noted with this property assignment.
#
# Multiple-Version compatibility was introduced in version 13.1;
# prior versions are therefore excluded.
set_sw_property min_compatible_hw_version 1.0

# Initialize the driver in alt_sys_init()
set_sw_property auto_initialize false

# Location in generated BSP that above sources will be copied into
set_sw_property bsp_subdirectory drivers

# Interrupt properties:
# This peripheral has an IRQ output but the driver doesn't currently
# have any interrupt service routine. To ensure that the BSP tools
# do not otherwise limit the BSP functionality for users of the
# Nios II enhanced interrupt port, these settings advertise 
# compliance with both legacy and enhanced interrupt APIs, and to state
# that any driver ISR supports preemption. If an interrupt handler
# is added to this driver, these must be re-examined for validity.
#set_sw_property isr_preemption_supported true
#set_sw_property supported_interrupt_apis "legacy_interrupt_api enhanced_interrupt_api"
set_sw_property isr_preemption_supported false
set_sw_property supported_interrupt_apis "legacy_interrupt_api enhanced_interrupt_api"


#
# Source file listings...
#

# Include files
add_sw_property include_source inc/tsd_time_input_selector_regs.h
add_sw_property include_source inc/tsd_time_input_selector.h
add_sw_property include_source ../tsd_register_common/tsd_register_common.h

add_sw_property include_directory ../tsd_register_common

# C source files
add_sw_property c_source src/tsd_time_input_selector.c

# This driver supports HAL & UCOSII BSP (OS) types
add_sw_property supported_bsp_type HAL
add_sw_property supported_bsp_type UCOSII

# System.h items
add_sw_setting boolean_define_only system_h_define \
tsd_time_input_selector_system_value TSD_TIME_INPUT_SELECTOR_DRIVER 1\
"Telspan Time Input Selector Driver System value"

# End of file
