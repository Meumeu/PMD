file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/models)

#TODO: detect blender folder, Windows compatibility
add_custom_command(OUTPUT models/Pony.mesh models/Pony.skeleton models/PonySkin.material models/PonyEye.material
	COMMAND blender-2.60 -b ${CMAKE_SOURCE_DIR}/blender/poney.blend -P "${CMAKE_SOURCE_DIR}/tools/io_export_ogreDotScene.py" -P "${CMAKE_SOURCE_DIR}/tools/export.py" -- models/Pony)

install(
	DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/models
	DESTINATION share/pmd
	PATTERN *.xml EXCLUDE)

add_custom_target(generate_models DEPENDS models/Pony.mesh models/Pony.skeleton models/PonySkin.material models/PonyEye.material)