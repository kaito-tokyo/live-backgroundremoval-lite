include_guard(GLOBAL)

set(
  CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO
  RelWithDebInfo
  Release
  MinSizeRel
  None
  ""
)
set(
  CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL
  MinSizeRel
  Release
  RelWithDebInfo
  None
  ""
)
set(
  CMAKE_MAP_IMPORTED_CONFIG_RELEASE
  Release
  RelWithDebInfo
  MinSizeRel
  None
  ""
)

set(BUILDSPEC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/buildspec.json")

if(EXISTS "${BUILDSPEC_FILE}")
  file(READ "${BUILDSPEC_FILE}" BUILDSPEC_CONTENT)

  string(JSON PLUGIN_NAME GET "${BUILDSPEC_CONTENT}" name)
  string(JSON PLUGIN_VERSION GET "${BUILDSPEC_CONTENT}" version)
  string(JSON PLUGIN_AUTHOR GET "${BUILDSPEC_CONTENT}" author)
  string(JSON PLUGIN_WEBSITE GET "${BUILDSPEC_CONTENT}" website)
  string(JSON PLUGIN_EMAIL GET "${BUILDSPEC_CONTENT}" email)

  string(JSON PREBUILT_VERSION GET "${BUILDSPEC_CONTENT}" dependencies prebuilt version)
  string(JSON QT6_VERSION GET "${BUILDSPEC_CONTENT}" dependencies qt6 version)

  string(JSON MACOS_BUNDLEID GET "${BUILDSPEC_CONTENT}" platformConfig macos bundleId)

  if(DEFINED ENV{GITHUB_RUN_NUMBER})
    set(PLUGIN_BUILD_NUMBER $ENV{GITHUB_RUN_NUMBER})
  else()
    set(PLUGIN_BUILD_NUMBER "1")
  endif()

  message(STATUS "Initialized Plugin: ${PLUGIN_NAME} v${PLUGIN_VERSION} (${PLUGIN_BUILD_NUMBER})")
else()
  message(FATAL_ERROR "Critical: buildspec.json not found at ${BUILDSPEC_FILE}")
endif()
