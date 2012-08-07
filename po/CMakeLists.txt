set(po_FILES
  de
)
set (ARK_LOCALE_DIR "/usr/share/locale/")

find_package(Gettext REQUIRED)

if (NOT GETTEXT_MSGMERGE_EXECUTABLE AND NOT GETTEXT_MSGFMT_EXECUTABLE)
  message(FATAL_ERROR "Gettext message catalog tools NOT found")
endif (NOT GETTEXT_MSGMERGE_EXECUTABLE AND NOT GETTEXT_MSGFMT_EXECUTABLE)

foreach(lang ${po_FILES})
  set(pot  "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/ark.pot")
  set(po  "${CMAKE_CURRENT_BINARY_DIR}/ark-${lang}.po")
  set(po_inst  "ark.po")


  # Create po files
  add_custom_target(all
    ${GETTEXT_MSGFMT_EXECUTABLE} -o ${po} ${pot}
    COMMENT "Create ${po}"
  )

  install(FILES ${po}
    DESTINATION "${ARK_LOCALE_DIR}/${lang}/LC_MESSAGES"
    RENAME ${po_inst}
  )
endforeach(lang)
