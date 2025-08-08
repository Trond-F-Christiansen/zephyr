#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
if ("${SB_CONFIG_CPURAD_BOARD}" STREQUAL "")
	message(FATAL_ERROR
	"Target ${BOARD} not supported for this sample. "
	"There is no remote board selected in Kconfig.sysbuild")
endif()

# Add cpurad project
ExternalZephyrProject_Add(
  APPLICATION cpurad_app
  SOURCE_DIR ${APP_DIR}/cpurad
  BOARD ${SB_CONFIG_CPURAD_BOARD}
  BOARD_REVISION ${BOARD_REVISION}
)

# Setup PM partitioning for cpurad
set_property(GLOBAL APPEND PROPERTY PM_DOMAINS CPURAD)
set_property(GLOBAL APPEND PROPERTY PM_CPURAD_IMAGES cpurad_app)
set_property(GLOBAL PROPERTY DOMAIN_APP_CPURAD cpurad_app)
set(CPURAD_PM_DOMAIN_DYNAMIC_PARTITION cpurad_app CACHE INTERNAL "")

sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} cpurad_app)
sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} cpurad_app)
  
