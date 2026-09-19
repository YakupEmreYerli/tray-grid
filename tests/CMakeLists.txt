# SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Test)

add_executable(gridtest gridtest.cpp mockwatcher.cpp mockwatcher.h)
target_link_libraries(gridtest PRIVATE Qt6::Widgets Qt6::DBus Qt6::Qml Qt6::Quick Qt6::Test)

# Runs against a package staged in the build tree, on a private session bus.
set(TRAYGRID_STAGE "${CMAKE_BINARY_DIR}/stage/${TRAYGRID_PLUGIN_ID}")
add_custom_target(stage-package ALL
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${TRAYGRID_STAGE}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/package ${TRAYGRID_STAGE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TRAYGRID_STAGE}/contents/lib
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:traygridplugin> ${CMAKE_SOURCE_DIR}/src/qmldir ${TRAYGRID_STAGE}/contents/lib/
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TRAYGRID_STAGE}/contents/icons
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/docs/brand/logo.svg ${TRAYGRID_STAGE}/contents/icons/tray-grid.svg
    DEPENDS traygridplugin
    COMMENT "Staging plasmoid package for tests")

find_program(DBUS_RUN_SESSION dbus-run-session REQUIRED)
add_test(NAME gridtest
    COMMAND ${DBUS_RUN_SESSION} -- $<TARGET_FILE:gridtest> ${TRAYGRID_STAGE} $<TARGET_FILE:traygrid-test-item> ${CMAKE_BINARY_DIR})
