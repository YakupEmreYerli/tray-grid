# SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# Core: StatusNotifierHost, item proxies and the DBusMenu client.
# Static (PIC) so both the QML plugin and the test tool can link it.
add_library(traygridcore STATIC
    sniimage.cpp sniimage.h
    snitypes.h
    statusnotifierhost.cpp statusnotifierhost.h
    statusnotifieritem.cpp statusnotifieritem.h
    dbusmenuclient.cpp dbusmenuclient.h
    popuphelper.cpp popuphelper.h
)
set_target_properties(traygridcore PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(traygridcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(traygridcore PUBLIC
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::DBus Qt6::Quick
    KF6::WindowSystem KF6::IconThemes
)

# QML plugin, loaded through a directory import from the plasmoid package.
# It is a plain QQmlExtensionPlugin (no qt_add_qml_module): the types are
# registered under whatever URI the directory import resolves to.
add_library(traygridplugin MODULE
    traygridplugin.cpp
    statusnotifiermodel.cpp statusnotifiermodel.h
)
target_link_libraries(traygridplugin PRIVATE traygridcore Qt6::Qml Qt6::Quick)
# qmldir says "plugin traygridplugin"; QML looks for libtraygridplugin.so.
set_target_properties(traygridplugin PROPERTIES PREFIX "lib" INSTALL_RPATH "")

set(TRAYGRID_PACKAGE_LIB_DIR "${KDE_INSTALL_DATADIR}/plasma/plasmoids/${TRAYGRID_PLUGIN_ID}/contents/lib")
install(TARGETS traygridplugin LIBRARY DESTINATION ${TRAYGRID_PACKAGE_LIB_DIR})
install(FILES qmldir DESTINATION ${TRAYGRID_PACKAGE_LIB_DIR})
