# SDK OTA state machine retained independently of the custom player view.
if(CONFIG_OTA_FOREGROUND)
  target_sources(app PRIVATE ota_app_foreground.c)
endif()
if(CONFIG_OTA_BACKGROUND)
  target_sources(app PRIVATE ota_app_background.c)
  target_sources_ifdef(CONFIG_UI_MANAGER app PRIVATE ota_msgbox_view.c)
endif()
