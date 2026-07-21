if(CONFIG_ZMK_ADS1015_INPUT)
  zephyr_library()
  zephyr_library_sources(src/ads1015_input.c)
endif()
