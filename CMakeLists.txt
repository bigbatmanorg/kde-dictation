cmake_minimum_required(VERSION 3.24)
project(kde-dictation VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GNUInstallDirs)
include(CTest)

option(KDICT_BUILD_TESTS "Build the unit tests" ON)
option(KDICT_INSTALL_AUTOSTART "Install a system-wide XDG autostart entry" OFF)

set(KDICT_QT_COMPONENTS Core Gui Widgets Network Multimedia WebSockets DBus)
if(BUILD_TESTING AND KDICT_BUILD_TESTS)
    list(APPEND KDICT_QT_COMPONENTS Test)
endif()
# Qt 6.6 is the floor of KDE Frameworks 6.0, the oldest KF6 this can run on.
find_package(Qt6 6.6 REQUIRED COMPONENTS ${KDICT_QT_COMPONENTS})
find_package(KF6GlobalAccel CONFIG REQUIRED)
find_package(KF6StatusNotifierItem CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

if(TARGET yaml-cpp::yaml-cpp)
    set(KDICT_YAML_TARGET yaml-cpp::yaml-cpp)
else()
    set(KDICT_YAML_TARGET yaml-cpp)
endif()

add_library(kde_dictation_core STATIC
    src/app/DictationState.cpp
    src/app/FallbackPolicy.cpp
    src/app/LiveTranscriptReconciler.cpp
    src/audio/DeviceSelection.cpp
    src/audio/WavEncoder.cpp
    src/config/Config.cpp
    src/output/PasteKeys.cpp
    src/output/YdotoolSocket.cpp
    src/stt/RealtimeEventParser.cpp
    src/util/Session.cpp
    src/util/UrlHelpers.cpp
)

target_include_directories(kde_dictation_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(kde_dictation_core
    PUBLIC
        Qt6::Core
        Qt6::Gui
        ${KDICT_YAML_TARGET}
)

target_compile_options(kde_dictation_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(kde-dictation
    src/main.cpp
    src/app/DictationController.cpp
    src/audio/AudioRecorder.cpp
    src/output/PasteBackend.cpp
    src/output/TextOutput.cpp
    src/output/XdotoolBackend.cpp
    src/output/YdotoolBackend.cpp
    src/shortcut/ShortcutController.cpp
    src/stt/BatchOpenAIProvider.cpp
    src/stt/RealtimeAsrProvider.cpp
    src/ui/TrayController.cpp
)

target_link_libraries(kde-dictation
    PRIVATE
        kde_dictation_core
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Multimedia
        Qt6::WebSockets
        Qt6::DBus
        KF6::GlobalAccel
        KF6::StatusNotifierItem
)

target_compile_options(kde-dictation PRIVATE -Wall -Wextra -Wpedantic)

target_compile_definitions(kde-dictation PRIVATE
    KDICT_VERSION="${PROJECT_VERSION}"
)

install(TARGETS kde-dictation RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/org.amvc.kdedictation.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/org.amvc.kdedictation.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES config.example.yaml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/kde-dictation)

if(KDICT_INSTALL_AUTOSTART)
    install(FILES packaging/org.amvc.kdedictation.autostart.desktop
            DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/xdg/autostart
            RENAME org.amvc.kdedictation.desktop)
endif()

if(BUILD_TESTING AND KDICT_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# `cpack -G DEB` / `cpack -G RPM` produce simple binary packages (CI uses these
# for the per-distro release artifacts; shared-library dependencies are
# derived automatically from the built binary).
set(CPACK_PACKAGE_NAME "kde-dictation")
set(CPACK_PACKAGE_VENDOR "bigbatmanorg")
set(CPACK_PACKAGE_CONTACT "moravcik.adam@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Global shortcut dictation client for KDE Plasma")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "ydotool, xdotool")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_AUTOREQ ON)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_SUGGESTS "ydotool xdotool")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/usr/share/applications;/usr/share/metainfo")
include(CPack)
