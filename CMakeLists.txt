# SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.22)
project(tray-grid VERSION 0.1.0 LANGUAGES CXX)

set(QT_MIN_VERSION 6.8.0)
set(KF_MIN_VERSION 6.10.0)

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui Widgets DBus Qml Quick)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS WindowSystem IconThemes)
find_package(Plasma REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TRAYGRID_PLUGIN_ID "com.github.yakupemreyerli.traygrid")

# The C++ QML plugin is shipped *inside* the plasmoid package (contents/lib)
# and imported from main.qml with a directory import. This keeps the
# installation per-user and self-contained: no QML_IMPORT_PATH, no relogin.
add_subdirectory(src)

plasma_install_package(package ${TRAYGRID_PLUGIN_ID})
# The package icon is referenced as "/contents/icons/tray-grid.svg" in
# metadata.json, which the widget explorer resolves relative to the package.
# The single source of truth is docs/brand/logo.svg.
install(FILES docs/brand/logo.svg
    DESTINATION ${KDE_INSTALL_DATADIR}/plasma/plasmoids/${TRAYGRID_PLUGIN_ID}/contents/icons
    RENAME tray-grid.svg)

option(BUILD_TESTING "Build the tools and the end-to-end test" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tools)
    add_subdirectory(tests)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
