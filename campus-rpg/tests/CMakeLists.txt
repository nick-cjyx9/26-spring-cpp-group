# Unit tests for CampusRPG core logic (pure C++, no SFML/SQLite dependency).
# Links only the pure-C++ core library (CampusRPGCore) and mock engine implementations.
set(TEST_SOURCES
    test_core.cpp
    mocks/MockRenderer.cpp
    mocks/MockInput.cpp
    mocks/MockWindow.cpp
)

add_executable(CampusRPGTests ${TEST_SOURCES})

target_include_directories(CampusRPGTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/engine/interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

target_link_libraries(CampusRPGTests PRIVATE
    CampusRPGCore
)

include(CTest)
add_test(NAME CampusRPGTests COMMAND CampusRPGTests)
