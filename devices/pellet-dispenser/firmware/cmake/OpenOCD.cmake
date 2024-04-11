find_program(OPENOCD_EXECUTABLE openocd)

if(OPENOCD_EXECUTABLE)
	execute_process(
		COMMAND ${OPENOCD_EXECUTABLE} "--version"
		ERROR_VARIABLE OCD_VERSION
		OUTPUT_QUIET
	)
	string(REGEX REPLACE ".*Open On-Chip Debugger ([0-9]+\\.[0-9]+\\.[0-9]+).*"
						 "\\1" OPENOCD_VERSION ${OCD_VERSION}
	)
endif()

if(NOT OPENOCD_EXECUTABLE OR OPENOCD_VERSION VERSION_LESS "0.12.0")
	if(OPENOCD_EXECUTABLE)
		message(
			STATUS
				"Found OpenOCD ${OPENOCD_VERSION} but it doesn't support rp2040. Downloading Pi foundation version"
		)
	endif()

	include(ProcessorCount)
	ProcessorCount(N)
	ExternalProject_Add(
		openocd
		PREFIX ${PROJECT_BINARY_DIR}/_deps/openocd
		GIT_REPOSITORY https://github.com/raspberrypi/openocd.git
		GIT_TAG rp2040-v0.12.0
		GIT_SHALLOW 1
		GIT_PROGRESS 1
		BUILD_IN_SOURCE 1
		UPDATE_DISCONNECTED 1
		CONFIGURE_COMMAND ./bootstrap
		COMMAND ./configure --prefix=${PROJECT_BINARY_DIR}/_deps/openocd-install
		BUILD_COMMAND make -j ${N}
		LOG_CONFIGURE 1
		LOG_OUTPUT_ON_FAILURE 1
	)
	set(OPENOCD_EXECUTABLE
		${PROJECT_BINARY_DIR}/_deps/openocd-install/bin/openocd
	)
	set(OPENOCD_VERSION "0.12.0")
endif()

add_custom_target(
	openocd-server COMMAND ${OPENOCD_EXECUTABLE} -f interface/cmsis-dap.cfg -c
						   "adapter speed 5000" -f target/rp2040.cfg
)

include(CMakeParseArguments)

function(add_openocd_upload_target)
	set(options)
	set(oneValueArgs TARGET)
	set(multiValueArgs)
	cmake_parse_arguments(
		ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
	)

	if(NOT ARGS_TARGET)
		message(FATAL_ERROR "You must specify one target to upload")
	endif()

	message(
		FATAL_ERROR "target is ${ARGS_TARGET} $<TARGET_FILE:${ARGS_TARGET}>"
	)

	add_custom_target(
		${ARGS_TARGET}-upload
		COMMAND
			${OPENOCD_EXECUTABLE} -f interface/cmsis-dap.cfg -c
			"adapter speed 5000" -f target/rp2040.cfg -c
			"program ${FOO} verify reset exit"
	)

endfunction()
