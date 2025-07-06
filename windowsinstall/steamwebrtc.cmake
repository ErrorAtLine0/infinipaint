
# Compute the installation prefix relative to this file.
get_filename_component(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
if(_IMPORT_PREFIX STREQUAL "/")
  set(_IMPORT_PREFIX "")
endif()

add_library(GameNetworkingSockets::steamwebrtc STATIC IMPORTED)
set_target_properties(GameNetworkingSockets::steamwebrtc PROPERTIES
	IMPORTED_LOCATION "${_IMPORT_PREFIX}/lib/steamwebrtc.lib"
)

# Cleanup temporary variables.
set(_IMPORT_PREFIX)
