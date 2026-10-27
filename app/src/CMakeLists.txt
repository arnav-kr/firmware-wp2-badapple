target_sources(app PRIVATE
  ui/player_view.c
  video/player.c
  video/gpu_metrics.c
  audio/codec.c
  audio/music.c
  audio/speaker.c
  input/crown.c
  "${WP2_GENERATED}/badapple_data.c"
  "${WP2_GENERATED}/badapple_audio_data.c")
