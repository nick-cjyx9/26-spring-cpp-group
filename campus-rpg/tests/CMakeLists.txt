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

# Save/persistence tests: exercise the SQLite-backed multi-slot SaveRepository.
# Links CampusRPGAppLib (data + manager + scenes; scenes only depend on engine
# interfaces, not SFML, so this test runs headless without an SFML link).
add_executable(CampusRPGSaveTests test_save.cpp)

target_include_directories(CampusRPGSaveTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/data
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/engine/interfaces
)

target_link_libraries(CampusRPGSaveTests PRIVATE
    CampusRPGAppLib
)

# Level-up scene tests: exercise LevelUpScene rendering and input with mocks.
add_executable(CampusRPGLevelUpTests
    test_levelup.cpp
    mocks/MockRenderer.cpp
    mocks/MockInput.cpp
    mocks/MockWindow.cpp
)

target_include_directories(CampusRPGLevelUpTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/data
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/engine/interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

target_link_libraries(CampusRPGLevelUpTests PRIVATE
    CampusRPGAppLib
)

include(CTest)
add_test(NAME CampusRPGTests COMMAND CampusRPGTests)
add_test(NAME CampusRPGSaveTests COMMAND CampusRPGSaveTests)
add_test(NAME CampusRPGLevelUpTests COMMAND CampusRPGLevelUpTests)
