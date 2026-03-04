# SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
# SPDX-FileCopyrightText: 2023 Julius Künzel <jk.kdedev@smartlab.uber.space>
# SPDX-License-Identifier: BSD-2-Clause

include(ECMAddTests)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
)

set(TESTS_LINK_LIBS
    Qt${QT_MAJOR_VERSION}::Test
    Qt${QT_MAJOR_VERSION}::Widgets
    KF${KF_MAJOR}::I18n
    QtColorWidgets
    Glaxnimate::Core
)

# Single source file tests, linking only to default libs

ecm_add_tests(
    test_bezier_solver.cpp
    test_riff.cpp
    LINK_LIBRARIES ${TESTS_LINK_LIBS}
)

# Single source file tests, linking to core lib additionally

ecm_add_tests(
    test_property.cpp
    test_trim_path.cpp
    test_trace.cpp
    test_aep_gradient_xml.cpp
    LINK_LIBRARIES ${TESTS_LINK_LIBS}
)

# Multi source files tests

ecm_add_test(
    test_bezier_length.cpp
    TEST_NAME test_bezier_length
    LINK_LIBRARIES ${TESTS_LINK_LIBS}
)

ecm_add_test(
    test_keyframe_transition.cpp
    TEST_NAME test_keyframe_transition
    LINK_LIBRARIES ${TESTS_LINK_LIBS}
)

ecm_add_test(
    test_cos_parser.cpp
    TEST_NAME test_cos_parser
    LINK_LIBRARIES ${TESTS_LINK_LIBS}
)
