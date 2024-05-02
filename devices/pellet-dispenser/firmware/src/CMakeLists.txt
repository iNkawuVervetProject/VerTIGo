add_custom_target(
	timestamp
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/timestamp.cmake
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

option(USB_INTERFACE "Adds USB interface to the firmware" On)

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
	DispenserController.cpp
	Log.cpp
	Error.cpp
	utils/Queue.cpp
	utils/Publisher.cpp
	utils/Processor.cpp
	LEDMorse.cpp
	ErrorReporter.cpp
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
	DispenserController.hpp
	Log.hpp
	utils/Queue.hpp
	utils/Publisher.hpp
	utils/Processor.hpp
	usb/callbacks.h
	LEDMorse.hpp
	ErrorReporter.hpp
)

if(USB_INTERFACE)
	set(USB_SRC_FILES usb/usb_descriptors.c usb/callbacks.cpp)
	set(USB_HDR_FILES usb/tusb_config.h usb/usb_descriptors.h)
	set(USB_LIBRARIES tinyusb_device tinyusb_board)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/usb ${CMAKE_CURRENT_BINARY_DIR})

add_executable(
	pellet-dispenser ${SRC_FILES} ${HDR_FILES} ${USB_SRC_FILES}
					 ${USB_HDR_FILES}
)

if(USB_INTERFACE)
	target_compile_definitions(pellet-dispenser PUBLIC "-DUSB_INTERFACE=1")
endif()

add_dependencies(pellet-dispenser timestamp)

target_compile_definitions(pellet-dispenser PUBLIC BOARD=pico)

target_include_directories(pellet-dispenser PUBLIC ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(
	pellet-dispenser
	pico_stdlib
	pico_multicore
	pico_util
	pico_unique_id
	pico_sync_core
	hardware_pio
	hardware_pwm
	hardware_flash
	${USB_LIBRARIES}
)

pico_generate_pio_header(
	pellet-dispenser ${CMAKE_CURRENT_SOURCE_DIR}/hardware/ir_sensor.pio
)

pico_add_extra_outputs(pellet-dispenser)

add_openocd_upload_target(TARGET pellet-dispenser)
