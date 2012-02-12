file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/models)

#TODO: detect blender folder, Windows compatibility
if(EXISTS /usr/bin/blender)
  set(BLENDER /usr/bin/blender)
elseif(EXISTS /usr/bin/blender-2.60)
  set(BLENDER /usr/bin/blender-2.60)
else()
  message(SEND_ERROR "Blender not found")
endif()

set(MODELS
	poney.blend
	wall.blend
	niveau.blend
	niveau2.blend)

set(MODELS_OUT)

foreach(MODEL ${MODELS})

get_filename_component(OUTNAME ${MODEL} NAME_WE)
set(OUTNAME models/${OUTNAME}.zip)
list(APPEND MODELS_OUT ${OUTNAME})

add_custom_command(OUTPUT ${OUTNAME}
	COMMAND ${BLENDER} -b ${CMAKE_SOURCE_DIR}/blender/${MODEL} -P "${CMAKE_SOURCE_DIR}/tools/io_export_ogreDotScene.py" -P "${CMAKE_SOURCE_DIR}/tools/export.py" -- ${OUTNAME}
	DEPENDS ${MODEL})

endforeach()

add_custom_target(generate_models DEPENDS ${MODELS_OUT})

install(
	DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/models
	DESTINATION share/pmd)

