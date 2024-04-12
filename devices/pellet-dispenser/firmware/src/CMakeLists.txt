set(SRC_FILES
	main.cpp #
	Display.cpp #
	Button.cpp #
	IRSensor.cpp
)
set(HDR_FILES
	Display.hpp #
	Button.hpp #
	IRSensor.hpp #
)

add_executable(pellet-dispenser ${SRC_FILES} ${HDR_FILES})

target_link_libraries(pellet-dispenser pico_stdlib pico_multicore pico_util)

pico_add_extra_outputs(pellet-dispenser)

add_openocd_upload_target(TARGET pellet-dispenser)
