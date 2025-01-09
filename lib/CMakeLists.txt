if(CONFIG_LIB_SIOT)
  zephyr_library()
  zephyr_library_sources(point.c html.c metrics.c zbus.c)
endif()
