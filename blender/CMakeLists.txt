file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/models)

#TODO: detect blender folder, Windows compatibility
if(EXISTS /usr/bin/blender)
  set(BLENDER /usr/bin/blender)
elseif(EXISTS /usr/bin/blender-2.60)
  set(BLENDER /usr/bin/blender-2.60)
else()
  message(SEND_ERROR "Blender not found")
endif()

add_custom_command(OUTPUT models/Pony.zip
	COMMAND ${BLENDER} -b ${CMAKE_SOURCE_DIR}/blender/poney.blend -P "${CMAKE_SOURCE_DIR}/tools/io_export_ogreDotScene.py" -P "${CMAKE_SOURCE_DIR}/tools/export.py" -- models/Pony.zip)

install(
	DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/models
	DESTINATION share/pmd)

add_custom_target(generate_models DEPENDS models/Pony.zip)
