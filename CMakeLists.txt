
add_library(
	freeram SHARED
	"src/freeram.cpp"
	"src/cJSON.c"
	"src/malloc.cpp"
)

add_executable(
	freeram_test
	"interface/freeram.c"
	"test.c"
)
if(NOT WIN32)
	# not needed on windows
	target_link_libraries(freeram_test dl)
	# on windows, i have a replacement for new/delete
	# on linux, listdc++ is always present anyways so don't bother
	target_link_libraries(freeram stdc++)
endif()
