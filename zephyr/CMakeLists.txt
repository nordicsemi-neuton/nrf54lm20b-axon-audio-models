#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/lib/nrf_edgeai/include)
zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/lib/axon/include)

zephyr_link_libraries(${ZEPHYR_CURRENT_MODULE_DIR}/lib/nrf_edgeai/lib/libnrf_edgeai_cortex-m33.a)

# Note: In Zephyr builds, the axon-ml SDK adds sources directly to Zephyr via zephyr_sources()
# and the SDK's driver CMakeLists.txt already handles linking nrf-axon-driver-internal.
# No additional zephyr_link_libraries() call is needed here.
add_subdirectory(
	${CMAKE_CURRENT_LIST_DIR}/../lib/axon/lib
	${CMAKE_CURRENT_BINARY_DIR}/lib_axon_lib
)
add_subdirectory(
	${CMAKE_CURRENT_LIST_DIR}/../lib/axon/drivers
	${CMAKE_CURRENT_BINARY_DIR}/lib_axon_drivers
)
