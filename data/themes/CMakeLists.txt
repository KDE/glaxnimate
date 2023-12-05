

set(SCHEME_FILES GlaxnimateDark.colors)

if ( NOT LINUX )
    list(APPEND SCHEME_FILES BreezeClassic.colors)
endif()

install(FILES ${SCHEME_FILES} DESTINATION ${DATA_INSTALL_DIR}/color-schemes)
