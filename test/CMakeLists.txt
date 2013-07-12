set(TEST_SRCS
test.c
tests_safeword_init.c
tests_safeword_add.c
tests_safeword_remove.c
tests_safeword_read.c
tests_safeword_list.c
tests_safeword_tag.c
)

# put the executable in the project root directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
add_executable(unittest ${TEST_SRCS})
#set_target_properties(unittest PROPERTIES OUTPUT_NAME test)
if(WIN32)
	set(LIBS safeword commands ${SQLITE3_LIBRARIES})
else()
	set(LIBS safeword commands ${SQLITE3_LIBRARIES} ${X11_LIBRARIES} ${X11_Xmu_LIB} rt)
endif()
target_link_libraries(unittest ${LIBS} ${CUNIT_LIBRARIES})
