cmake_minimum_required(VERSION 3.20)
project(CampusRPG
    VERSION 1.0.0
    DESCRIPTION "Campus RPG 2D Adventure Game System"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Options ----
# Allow advanced users to skip FetchContent and link against a local install.
option(CAMPUS_RPG_USE_SYSTEM_SFML "Use system/installed SFML instead of FetchContent" OFF)
option(CAMPUS_RPG_USE_SYSTEM_SQLITE3 "Use system/installed SQLite3 instead of FetchContent" OFF)
option(CAMPUS_RPG_BUILD_TESTS "Build the CampusRPGTests target" ON)

# ---- Dependencies ----
include(FetchContent)

# SFML
if(CAMPUS_RPG_USE_SYSTEM_SFML)
    find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)
else()
    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SFML)
endif()

# SQLite3
if(CAMPUS_RPG_USE_SYSTEM_SQLITE3)
    find_package(SQLite3 REQUIRED)
    # Provide a namespaced target alias if the system package did not create one.
    if(NOT TARGET SQLite::SQLite3 AND TARGET SQLite3::SQLite3)
        add_library(SQLite::SQLite3 ALIAS SQLite3::SQLite3)
    endif()
else()
    FetchContent_Declare(
        sqlite3
        URL https://www.sqlite.org/2024/sqlite-amalgamation-3450200.zip
    )
    FetchContent_MakeAvailable(sqlite3)

    # SQLite3 amalgamation provides sqlite3.c / sqlite3.h.
    find_package(Threads REQUIRED)
    add_library(SQLite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
    target_include_directories(SQLite3 PUBLIC ${sqlite3_SOURCE_DIR})
    target_link_libraries(SQLite3 PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
endif()

# ---- Core static library (pure C++ domain models, no SFML/SQLite) ----
set(CORE_SOURCES
    src/core/Character.cpp
    src/core/Item.cpp
    src/core/Inventory.cpp
    src/core/Quest.cpp
    src/core/QuestManager.cpp
    src/core/Enemy.cpp
    src/core/Shop.cpp
    src/core/BattleSystem.cpp
    src/core/Persona.cpp
    src/core/Skill.cpp
    src/core/SocialLink.cpp
    src/core/SocialLinkManager.cpp
    src/core/Entity.cpp
    src/core/TileMap.cpp
)

add_library(CampusRPGCore STATIC ${CORE_SOURCES})
target_include_directories(CampusRPGCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/interfaces
)

# ---- Engine interface library ----
add_library(CampusRPGEngineInterface INTERFACE)
target_include_directories(CampusRPGEngineInterface INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/interfaces
)

# ---- App library (data + manager + scenes) ----
set(APP_LIB_SOURCES
    src/data/DatabaseManager.cpp
    src/data/SaveRepository.cpp
    src/manager/GameManager.cpp
    src/scenes/TitleScene.cpp
    src/scenes/TownScene.cpp
    src/scenes/NightScene.cpp
    src/scenes/BattleScene.cpp
    src/scenes/ShopScene.cpp
    src/scenes/InventoryScene.cpp
    src/scenes/CharacterScene.cpp
    src/scenes/DialogueScene.cpp
    src/scenes/SaveSlotScene.cpp
    src/scenes/StatusScene.cpp
    src/scenes/ArmoryScene.cpp
)

add_library(CampusRPGAppLib STATIC ${APP_LIB_SOURCES})
target_include_directories(CampusRPGAppLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data
    ${CMAKE_CURRENT_SOURCE_DIR}/src/manager
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scenes
)
target_link_libraries(CampusRPGAppLib PUBLIC
    CampusRPGCore
    CampusRPGEngineInterface
    $<IF:$<BOOL:${CAMPUS_RPG_USE_SYSTEM_SQLITE3}>,SQLite::SQLite3,SQLite3>
)

# ---- Main executable (SFML implementation) ----
set(ENGINE_SFML_SOURCES
    src/engine/sfml/SfmlInput.cpp
    src/engine/sfml/SfmlRenderer.cpp
    src/engine/sfml/SfmlWindow.cpp
)

add_executable(${PROJECT_NAME}
    src/main.cpp
    ${ENGINE_SFML_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/sfml
)

if(CAMPUS_RPG_USE_SYSTEM_SFML)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        CampusRPGAppLib
        SFML::graphics
        SFML::window
        SFML::system
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        CampusRPGAppLib
        sfml-graphics
        sfml-window
        sfml-system
    )
endif()

# Copy lightweight runtime resources next to the executable.
# Note: copy_directory_if_different requires CMake 3.26+; use copy_directory for 3.20 compatibility.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/textures
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources/textures)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources/fonts)

# Copy SFML DLLs next to the executable on Windows so the app can run without extra PATH setup.
if(WIN32 AND NOT CAMPUS_RPG_USE_SYSTEM_SFML)
    foreach(sfml_target sfml-graphics sfml-window sfml-system)
        if(TARGET ${sfml_target})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:${sfml_target}>
                    $<TARGET_FILE_DIR:${PROJECT_NAME}>)
        endif()
    endforeach()
endif()

# ---- Testing ----
if(CAMPUS_RPG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
