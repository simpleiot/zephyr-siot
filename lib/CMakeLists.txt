if(CONFIG_LIB_SIOT)
  zephyr_library()
  zephyr_library_sources(
    point.c
    html.c
    metrics.c
    zbus.c
    nvs.c
    siot-string.c
    timeconv.c
  )
  zephyr_library_sources_ifdef(CONFIG_SIOT_POINT_CACHE point-cache.c)
endif()
