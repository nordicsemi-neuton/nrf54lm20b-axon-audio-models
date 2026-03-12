#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (${CONFIG_NRF_AXON_MFCC})
	add_subdirectory(mfcc)
endif()

include(axon_internal_cmake.inc OPTIONAL) 

