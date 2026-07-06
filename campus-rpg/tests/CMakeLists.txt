# Unit tests for CampusRPG core logic (pure C++, no Qt dependency).
# Links only the pure-C++ core library (CampusRPGCore), so the test binary
# runs anywhere without a Qt event loop or platform plugin.
set(TEST_SOURCES
    test_core.cpp
)

add_executable(CampusRPGTests ${TEST_SOURCES})

target_link_libraries(CampusRPGTests PRIVATE
    CampusRPGCore
)

include(CTest)
add_test(NAME CampusRPGTests COMMAND CampusRPGTests)
