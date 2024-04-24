set(SRC_FILES
	main.cpp #
	Display.cpp #
	Button.cpp #
	hardware/IRSensor.cpp #
	hardware/PIOIRSensor.cpp
	hardware/DRV8848.cpp
	WheelController.cpp
	PelletCounter.cpp
	hardware/IRQManager.cpp
	hardware/FlashStorage.cpp
	usb/usb_descriptors.c
)
set(HDR_FILES
	Display.hpp #
	Button.hpp #
	hardware/IRSensor.hpp #
	hardware/PIOIRSensor.hpp
	hardware/DRV8848.hpp
	WheelController.hpp
	PelletCounter.hpp
	hardware/IRQManager.hpp
	hardware/FlashStorage.hpp
	utils/Defer.hpp
	usb/tusb_config.h
	usb/usb_descriptors.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/usb)

add_executable(pellet-dispenser ${SRC_FILES} ${HDR_FILES})

target_compile_definitions(pellet-dispenser PUBLIC BOARD=pico)

target_link_libraries(
	pellet-dispenser
	pico_stdlib
	pico_multicore
	pico_util
	pico_unique_id
	hardware_pio
	hardware_pwm
	hardware_flash
	tinyusb_device
	tinyusb_board
)

pico_generate_pio_header(
	pellet-dispenser ${CMAKE_CURRENT_SOURCE_DIR}/hardware/ir_sensor.pio
)

pico_add_extra_outputs(pellet-dispenser)

add_openocd_upload_target(TARGET pellet-dispenser)
