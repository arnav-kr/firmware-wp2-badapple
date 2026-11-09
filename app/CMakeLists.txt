# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wp2_badapple)

if(TARGET freetype)
  get_target_property(WP2_FREETYPE_IMPORTED freetype IMPORTED)
  if(NOT WP2_FREETYPE_IMPORTED)
    set_target_properties(freetype PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
  endif()
endif()

get_filename_component(WP2_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(WP2_BOARD "${WP2_ROOT}/board/wp2")
set(WP2_DRIVERS "${WP2_ROOT}/drivers")
set(WP2_GENERATED "${WP2_ROOT}/generated")
set(WP2_VENDOR "${WP2_ROOT}/vendor" CACHE PATH "Local vendor inputs")

foreach(asset badapple_data.c badapple_audio_data.c)
  if(NOT EXISTS "${WP2_GENERATED}/${asset}")
    message(FATAL_ERROR "Missing generated/${asset}; run scripts/media.py embed first")
  endif()
endforeach()
if(NOT EXISTS "${WP2_VENDOR}/libbt_test.a")
  message(FATAL_ERROR "Supply vendor/libbt_test.a from the matching Actions SDK")
endif()

# Wrapper headers extend the matching SDK board without editing SDK files.
set(WP2_SDK_HEADERS "${CMAKE_CURRENT_BINARY_DIR}/sdk_headers")
file(MAKE_DIRECTORY "${WP2_SDK_HEADERS}")
foreach(header board.h board_cfg.h)
  file(WRITE "${WP2_SDK_HEADERS}/wp2_sdk_${header}"
    "#include \"${ZEPHYR_BASE}/boards/arm/ats3089c_dev_watch/${header}\"\n")
endforeach()
target_include_directories(zephyr_interface BEFORE INTERFACE
  "${WP2_BOARD}/include" "${WP2_SDK_HEADERS}")
target_include_directories(zephyr PRIVATE
  "${ZEPHYR_BASE}/drivers/display/panel" "${WP2_DRIVERS}/display")
target_include_directories(drivers__audio__phy_leopard PRIVATE
  "${ZEPHYR_BASE}/drivers/audio")

function(wp2_replace_source target original replacement)
  get_target_property(sources ${target} SOURCES)
  list(FILTER sources EXCLUDE REGEX "${original}$")
  set_property(TARGET ${target} PROPERTY SOURCES ${sources})
  target_sources(${target} PRIVATE "${WP2_ROOT}/${replacement}")
endfunction()

wp2_replace_source(drivers__input cst820_tpkey_acts\\.c drivers/input/touch.c)
wp2_replace_source(..__framework__audio audio_track\\.c drivers/audio/audio_track.c)
wp2_replace_source(drivers__i2c i2c_acts\\.c drivers/i2c/i2c.c)
wp2_replace_source(drivers__audio__phy_leopard phy_audio_dac\\.c drivers/audio/dac.c)
target_sources(zephyr PRIVATE "${WP2_DRIVERS}/display/lifecycle.c" "${WP2_DRIVERS}/display/panel.c")
set_property(TARGET boards__arm__ats3089c_dev_watch PROPERTY SOURCES
  "${WP2_BOARD}/board.c")
if(CONFIG_ACTS_LEOPARD_BATTERY_SUPPLY_INTERNAL)
  set_property(TARGET drivers__power_supply__battery_leopard_internal PROPERTY SOURCES
    "${WP2_DRIVERS}/power/bat_charge_driver.c"
    "${WP2_DRIVERS}/power/bat_charge_ctrl.c"
    "${WP2_DRIVERS}/power/bat_charge_status.c")
endif()

target_compile_definitions(app PRIVATE PRO2_INPUT5 PRO2_MEDIA11)
target_include_directories(app PRIVATE
  src/audio src/input src/video sdk/ota
  "${WP2_DRIVERS}/display" "${WP2_ROOT}/third_party/minimp3"
  prebuild/bt_production_test)
target_link_libraries(app INTERFACE "${WP2_VENDOR}/libbt_test.a")
zephyr_ld_options(-Wl,--wrap=vg_lite_blit -Wl,--wrap=vg_lite_blit_rect)
add_subdirectory(sdk)
add_subdirectory(src)
if(CONFIG_STACK_CANARIES)
  zephyr_library_compile_options(-DAPP_BT_WATCH -fstack-protector-all)
endif()
