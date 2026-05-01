#
# Vitesse Switch Software.
#
# Copyright (c) 2002-2011 Vitesse Semiconductor Corporation "Vitesse". All
# Rights Reserved.
#
# Unpublished rights reserved under the copyright laws of the United States of
# America, other countries and international treaties. Permission to use, copy,
# store and modify, the software and its source code is granted. Permission to
# integrate into other products, disclose, transmit and distribute the software
# in an absolute machine readable format (e.g. HEX file) is also granted.  The
# source code of the software may not be disclosed, transmitted or distributed
# without the written permission of Vitesse. The software and its source code
# may only be used in products utilizing the Vitesse switch products.
#
# This copyright notice must appear in any copy, modification, disclosure,
# transmission or distribution of the software. Vitesse retains all ownership,
# copyright, trade secret and proprietary rights in the software.
#
# THIS SOFTWARE HAS BEEN PROVIDED "AS IS," WITHOUT EXPRESS OR IMPLIED WARRANTY
# INCLUDING, WITHOUT LIMITATION, IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR USE AND NON-INFRINGEMENT.
#
include $(BUILD)/make/templates/eCosSwitch.in

# Custom/eCosConfig := debug
Custom/SwitchName := PHY-1588

$(eval $(call eCosSwitch/Luton26,CESERVICES,CARACAL_1,BOARD_LUTON10_REF))

# The minimum modules required for a build:
MODULES := board cli conf ecos main msg misc port sprout util vtss_api firmware ip packet sysutil vcli web

# Extra modules needed for the PHY Simulator:
MODULES += acl auth cli_telnet mac phy_1588_sim ssh ssh_lib symreg tod vlan

$(eval $(call eCosSwitch/Build))

