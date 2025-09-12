if(NOT $ENV{VCPKG_ROOT} STREQUAL "")
  set(VCPKG_ROOT $ENV{VCPKG_ROOT})
elseif(NOT $ENV{VCPKG_INSTALLATION_ROOT} STREQUAL "")
  set(VCPKG_ROOT $ENV{VCPKG_INSTALLATION_ROOT})
else()
  message(FATAL_ERROR "VCPKG_ROOT or VCPKG_INSTALLATION_ROOT environment variable is not set")
endif()

if(OS_MACOS)
  message(STATUS "macOS detected. Running vcpkg setup script...")
  execute_process(
    COMMAND bash "${CMAKE_SOURCE_DIR}/.github/scripts/install-vcpkg-macos.bash"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE _vcpkg_result
  )
  if(NOT _vcpkg_result EQUAL 0)
    message(FATAL_ERROR "vcpkg macOS script failed")
  endif()
else()
  message(STATUS "Windows/Linux detected. Running 'vcpkg install'...")
  execute_process(
    COMMAND ${VCPKG_ROOT}/vcpkg install --triplet=${VCPKG_TARGET_TRIPLET}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE _vcpkg_result
  )
  if(NOT _vcpkg_result EQUAL 0)
    message(FATAL_ERROR "vcpkg install failed")
  endif()
endif()

set(VCPKG_INSTALLED_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed")
list(PREPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
