configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

add_executable(gf_tileset
  gf_tileset.cc

  bits/TilesetApp.cc
  bits/TilesetData.cc
#   bits/TilesetDisplay.cc
  bits/TilesetGeneration.cc
  bits/TilesetGui.cc
  bits/TilesetProcess.cc
  bits/TilesetScene.cc
#   bits/TilesetState.cc

  ../../vendor/gf-imgui/imgui_impl_gf.cc
  ../../vendor/gf-imgui/imgui/imgui.cpp
  ../../vendor/gf-imgui/imgui/imgui_demo.cpp
  ../../vendor/gf-imgui/imgui/imgui_draw.cpp
  ../../vendor/gf-imgui/imgui/imgui_tables.cpp
  ../../vendor/gf-imgui/imgui/imgui_widgets.cpp
)

target_compile_features(gf_tileset
  PUBLIC
    cxx_std_14
)

set_target_properties(gf_tileset
  PROPERTIES
    CXX_EXTENSIONS OFF
)

target_include_directories(gf_tileset
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/gf-imgui/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/gf-imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/json/single_include"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(gf_tileset
  PRIVATE
    gf::graphics
)

install(
  TARGETS gf_tileset
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
