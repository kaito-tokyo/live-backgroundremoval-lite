if(OS_MACOS)
  message(STATUS "macOS detected. Running nuget setup script...")
  execute_process(
    COMMAND bash "${CMAKE_SOURCE_DIR}/.github/scripts/install-nuget-macos.bash"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE _nuget_result
  )
  if(NOT _nuget_result EQUAL 0)
    message(FATAL_ERROR "nuget macOS script failed")
  endif()
endif()

set(NUGET_INSTALLED_DIR "${CMAKE_SOURCE_DIR}/nuget_installed")
list(PREPEND CMAKE_PREFIX_PATH "${NUGET_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
