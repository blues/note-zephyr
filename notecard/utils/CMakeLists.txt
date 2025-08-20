# Extract git version information for NOTE_ZEPHYR_VERSION
execute_process(
	COMMAND git describe --tags --always --dirty
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../..
	OUTPUT_VARIABLE NOTE_ZEPHYR_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE
	ERROR_QUIET
)

# Fallback to commit hash if no tags are available
if(NOT NOTE_ZEPHYR_VERSION)
	execute_process(
		COMMAND git rev-parse --short HEAD
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../..
		OUTPUT_VARIABLE NOTE_ZEPHYR_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET
	)
endif()

# Final fallback if git is not available
if(NOT NOTE_ZEPHYR_VERSION)
	set(NOTE_ZEPHYR_VERSION "unknown")
endif()

# Define the version macro for compilation
zephyr_compile_definitions(NOTE_ZEPHYR_VERSION="${NOTE_ZEPHYR_VERSION}")

zephyr_library_sources(
	ua.c
)
