add_executable(test_fe test_fe.c ${PROJECT_SOURCE_DIR}/ae.c ${PROJECT_SOURCE_DIR}/zmalloc.c)

add_executable(test_sds test_sds.c ${PROJECT_SOURCE_DIR}/sds.c ${PROJECT_SOURCE_DIR}/zmalloc.c)

add_executable(test_dict test_dict.c ${PROJECT_SOURCE_DIR}/dict.c ${PROJECT_SOURCE_DIR}/zmalloc.c)
