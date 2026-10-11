################################################################################
# \file Makefile
# \version 1.0
#
# \brief Top-level application make file.
#
################################################################################
# \copyright
# (c) 2025-2026, Infineon Technologies AG, or an affiliate of Infineon
# Technologies AG. All rights reserved.
# This software, associated documentation and materials ("Software") is
# owned by Infineon Technologies AG or one of its affiliates ("Infineon")
# and is protected by and subject to worldwide patent protection, worldwide
# copyright laws, and international treaty provisions. Therefore, you may use
# this Software only as provided in the license agreement accompanying the
# software package from which you obtained this Software. If no license
# agreement applies, then any use, reproduction, modification, translation, or
# compilation of this Software is prohibited without the express written
# permission of Infineon.
# 
# Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
# IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
# THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
# SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
# Infineon reserves the right to make changes to the Software without notice.
# You are responsible for properly designing, programming, and testing the
# functionality and safety of your intended application of the Software, as
# well as complying with any legal requirements related to its use. Infineon
# does not guarantee that the Software will be free from intrusion, data theft
# or loss, or other breaches ("Security Breaches"), and Infineon shall have
# no liability arising out of any Security Breaches. Unless otherwise
# explicitly approved by Infineon, the Software may not be used in any
# application where a failure of the Product or any consequences of the use
# thereof can reasonably be expected to result in personal injury.
################################################################################


################################################################################
# Basic Configuration
################################################################################

# Type of ModusToolbox Makefile Options include:
#
# COMBINED    -- Top Level Makefile usually for single standalone application
# APPLICATION -- Top Level Makefile usually for multi project application
# PROJECT     -- Project Makefile under Application
#
MTB_TYPE=COMBINED

# Target board/hardware (BSP).
# To change the target, it is required to use the Library manager
# ('make library-manager' from command line), which will also update Eclipse IDE launch
# configurations.

TARGET=REF_PMG1_B2_DRP

# Name of application (used to derive name of final linked file).
#
# If APPNAME is edited, ensure to update or regenerate launch
# configurations for your IDE.
APPNAME=mtb-example-pmg1b2-usbpd-sink

# Name of toolchain to use. Options include:
#
# GCC_ARM -- GCC is available as part of ModusToolbox Setup program
# ARM     -- ARM Compiler (must be installed separately)
# IAR     -- IAR Compiler (must be installed separately)
#
# See also: CY_COMPILER_PATH below
TOOLCHAIN=GCC_ARM

# Default build configuration. Options include:
#
# Debug -- build with minimal optimizations, focus on debugging.
# Release -- build with full optimizations
# Custom -- build with custom configuration, set the optimization flag in CFLAGS
#
# If CONFIG is manually edited, ensure to update or regenerate launch configurations
# for your IDE.
CONFIG=Release

# If set to "true" or "1", display full command-lines when building.
VERBOSE=


################################################################################
# Advanced Configuration
################################################################################

# Enable optional code that is ordinarily disabled by default.
#
# Available components depend on the specific targeted hardware and firmware
# in use. In general, if you have
#
#    COMPONENTS=foo bar
#
# ... then code in directories named COMPONENT_foo and COMPONENT_bar will be
# added to the build
#
COMPONENTS=PMG1_PD3_SNK_EPR

# Like COMPONENTS, but disable optional code that was enabled by default.
DISABLE_COMPONENTS=

# By default the build system automatically looks in the Makefile's directory
# tree for source code and builds it. The SOURCES variable can be used to
# manually add source code to the build process from a location not searched
# by default, or otherwise not found by the build system.
SOURCES=

# Like SOURCES, but for include directories. Value should be paths to
# directories (without a leading -I).
INCLUDES=

# Add additional defines to the build process (without a leading -D).
# Enabled PD revision 3.0 support, VBus OV Fault Protection and Deep Sleep mode in idle states.
DEFINES=CY_PD_SINK_ONLY=1 MINOR_SVDM_VER_SUPPORT=1 SYS_DEEPSLEEP_ENABLE=1

PDSTACK_DEFINES=CY_PD_REV3_ENABLE=1 CY_PD_EPR_ENABLE=1 CY_PD_EPR_AVS_ENABLE=1

FAULT_DEFINES=CC_OVP_ENABLE=0 VBUS_OVP_ENABLE=1

SOLUTION_DEFINES=RDO_MAXCUR_OVERRIDE=1 VBUS_EXTERNAL_DISCHARGE=1 CY_APP_LED_CONTROL_ENABLE=1
SOLUTION_DEFINES += DEBUG_PRINT_ENABLE=0 DEBUG_PRINT_LOG_ENABLE=1 CY_PDUTILS_TIMER_NUM_TIMERS=31

SOLUTION_DEFINES += VOUT_UVP_VOLT=16000 # VOUT UVP threshold in mV
SOLUTION_DEFINES += VOUT_OVP_VOLT=28000 # VOUT OVP threshold in mV
SOLUTION_DEFINES += VOUT_INIT_MAX_VOLT=2000 # Max VOUT allowed before BB init in mV
SOLUTION_DEFINES += TYPEC_MIN_PWR=15000 # minimal TYPEC contract power BB requires in mW
SOLUTION_DEFINES += VBUS_OCP_CURR_PER=20 # VBUS OCP threshold in %
SOLUTION_DEFINES += NTC1_OTP_THRES=85 # NTC1 OTP level in C
SOLUTION_DEFINES += NTC1_TEMP_HYST=10 # NTC1 temperature hysteresis in C

# BJT average ADC code - Temperature C table
# 41 - 0 C
# 39 - 25 C
# 37 - 60 C
# 36 - 70 C
# 35 - 85 C
# 34 - 100 C
# 33 - 110 C
# 32 - 125 C
SOLUTION_DEFINES += BJT_OTP_CODE=34 # 100C; Variation: 85C to 115C
SOLUTION_DEFINES += BJT_HYST_CODE=1 # hysteresis in ADC BJT code
SOLUTION_DEFINES += LED_PD_SNK_TIMER_PERIOD=100 # LED toggle period (ms) when BB works
SOLUTION_DEFINES += BB_MONITOR_TASK_TIMER_PERIOD=100 # Parameters monitor and FSM period (ms)
SOLUTION_DEFINES += SOFTSTART_FET_IN_TIMER_PERIOD=1200 # Input FET soft-start enable delay (ms)
SOLUTION_DEFINES += PWMI_TIMER_PERIOD=5 # Delay between RECT low and PWMI high (ms)
SOLUTION_DEFINES += RECT_TIMER_PERIOD=15 # After BB start blocking reverse current (ms)

# Board specific definitions:
BOARD_DEFINES += RVL2=9100 # in Ohm
BOARD_DEFINES += RIN=9 # R sense in mOhm
BOARD_DEFINES += RVH2=45900 # in Ohm
BOARD_DEFINES += RFB=15 # in mOhm
BOARD_DEFINES += NTC1_B=4509.0 # thermistor B coefficient in Kelvin
BOARD_DEFINES += NTC1_RP=40000.0 # Pull-up resistor in thermistor divider in Ohm
BOARD_DEFINES += NTC1_T25=298.0 # temperature in Kelvin (273.0 + 25.0) for nominal thermistor resistance */
BOARD_DEFINES += NTC1_R25=100000.0 # thermistor resistance at 25C in Ohm

DEFINES +=$(PDSTACK_DEFINES)
DEFINES +=$(FAULT_DEFINES)
DEFINES +=$(SOLUTION_DEFINES)
DEFINES +=$(BOARD_DEFINES)

# Select softfp or hardfp floating point. Default is softfp.
VFP_SELECT=

# Additional / custom C compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CFLAGS=

# Additional / custom C++ compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CXXFLAGS=

# Additional / custom assembler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
ASFLAGS=

# Additional / custom linker flags.
ifeq ($(TOOLCHAIN), GCC_ARM)
LDFLAGS=-Wl,--no-warn-rwx-segments
else
LDFLAGS=
endif

# Additional / custom libraries to link in to the application.
LDLIBS=

# Path to the linker script to use (if empty, use the default linker script).
LINKER_SCRIPT=

# Custom pre-build commands to run.
PREBUILD=

# Custom post-build commands to run.
POSTBUILD=

# Variable set to "PMG1" to enable support for the PMG1 parts and BSPs.
export CY_SUPPORTED_KITS=PMG1

################################################################################
# Paths
################################################################################

# Relative path to the project directory (default is the Makefile's directory).
#
# This controls where automatic source code discovery looks for code.
CY_APP_PATH=

# Relative path to the shared repo location.
#
# All .mtb files have the format, <URI>#<COMMIT>#<LOCATION>. If the <LOCATION> field
# begins with $$ASSET_REPO$$, then the repo is deposited in the path specified by
# the CY_GETLIBS_SHARED_PATH variable. The default location is one directory level
# above the current app directory.
# This is used with CY_GETLIBS_SHARED_NAME variable, which specifies the directory name.
CY_GETLIBS_SHARED_PATH=../

# Directory name of the shared repo location.
#
CY_GETLIBS_SHARED_NAME=mtb_shared

# Absolute path to the compiler's "bin" directory. The variable name depends on the
# toolchain used for the build. Refer to the ModusToolbox user guide to get the correct
# variable name for the toolchain used in your build.
#
# The default path depends on the selected TOOLCHAIN and is set in the Make recipe.
CY_COMPILER_GCC_ARM_DIR?=


# Locate ModusToolbox helper tools folders in default installation
# locations for Windows, Linux, and macOS.
CY_WIN_HOME=$(subst \,/,$(USERPROFILE))
CY_TOOLS_PATHS ?= $(wildcard \
    $(CY_WIN_HOME)/ModusToolbox/tools_* \
    $(HOME)/ModusToolbox/tools_* \
    /Applications/ModusToolbox/tools_*)

# If you install ModusToolbox software in a custom location, add the path to its
# "tools_X.Y" folder (where X and Y are the version number of the tools
# folder). Make sure you use forward slashes.
CY_TOOLS_PATHS+=

# Default to the newest installed tools folder, or the users override (if it's
# found).
CY_TOOLS_DIR=$(lastword $(sort $(wildcard $(CY_TOOLS_PATHS))))

ifeq ($(CY_TOOLS_DIR),)
$(error Unable to find any of the available CY_TOOLS_PATHS -- $(CY_TOOLS_PATHS). On Windows, use forward slashes.)
endif

$(info Tools Directory: $(CY_TOOLS_DIR))

include $(CY_TOOLS_DIR)/make/start.mk
