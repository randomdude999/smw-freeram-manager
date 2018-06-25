
add_library(
	freeram SHARED
	"src/freeram.cpp"
	"src/cJSON.c"
)

add_executable(
	freeram_test
	"interface/freeram.c"
	"test.c"
)
target_link_libraries(freeram_test dl)