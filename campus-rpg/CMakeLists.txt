cmake_minimum_required(VERSION 3.20)
project(CampusRPG
    VERSION 1.0.0
    DESCRIPTION "Campus RPG Adventure Game System"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE/IntelliSense (VS Code C/C++ / clangd).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable Qt meta-object compiler / resource compiler / UI compiler
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src/ui)

# Find Qt6 components.
find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Test)

# ---- Core static library (pure C++ domain models, no Qt/Sql) ----
# Kept free of Qt/SQLite so unit tests stay fast and portable.
set(CORE_SOURCES
    src/core/Character.cpp
    src/core/Item.cpp
    src/core/Inventory.cpp
    src/core/Quest.cpp
    src/core/QuestManager.cpp
    src/core/Enemy.cpp
    src/core/Shop.cpp
    src/core/BattleSystem.cpp
)

add_library(CampusRPGCore STATIC ${CORE_SOURCES})
target_include_directories(CampusRPGCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

# ---- App library (data + manager: SQLite persistence + state coordinator) ----
set(APP_LIB_SOURCES
    src/data/DatabaseManager.cpp
    src/data/SaveRepository.cpp
    src/manager/GameManager.cpp
)

add_library(CampusRPGAppLib STATIC ${APP_LIB_SOURCES})
target_include_directories(CampusRPGAppLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data
    ${CMAKE_CURRENT_SOURCE_DIR}/src/manager
)
target_link_libraries(CampusRPGAppLib PUBLIC
    CampusRPGCore
    Qt6::Core
    Qt6::Sql
)

# ---- Main executable ----
set(UI_SOURCES
    src/ui/MainWindow.cpp
    src/ui/CharacterPage.cpp
    src/ui/InventoryPage.cpp
    src/ui/ShopPage.cpp
    src/ui/QuestPage.cpp
    src/ui/BattlePage.cpp
)

set(UI_FORMS
    src/ui/MainWindow.ui
    src/ui/CharacterPage.ui
    src/ui/InventoryPage.ui
    src/ui/ShopPage.ui
    src/ui/QuestPage.ui
    src/ui/BattlePage.ui
)

add_executable(${PROJECT_NAME}
    src/main.cpp
    ${UI_SOURCES}
    ${UI_FORMS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    CampusRPGAppLib
    Qt6::Widgets
)

# ---- Testing ----
enable_testing()
add_subdirectory(tests)
