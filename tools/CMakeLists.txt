# SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# Command line tool that runs the same host code as the plasmoid and prints
# what it sees. Not installed; used for testing and bug reports.
add_executable(sni-dump sni-dump.cpp)
target_link_libraries(sni-dump PRIVATE traygridcore)

# A controllable StatusNotifierItem used by the tests.
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS StatusNotifierItem)
add_executable(traygrid-test-item test-item.cpp)
target_link_libraries(traygrid-test-item PRIVATE Qt6::Widgets KF6::StatusNotifierItem)
