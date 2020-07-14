configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

add_executable(gf_dungeons
  gf_dungeons.cc
  bits/DungeonApp.cc
  bits/DungeonDisplay.cc
  bits/DungeonGenerator.cc
  bits/DungeonGenerator_BinarySpacePartitioning.cc
  bits/DungeonGenerator_CellularAutomaton.cc
  bits/DungeonGenerator_DrunkardMarch.cc
  bits/DungeonGenerator_Tunneling.cc
  bits/DungeonGui.cc
  bits/DungeonScene.cc
  bits/DungeonState.cc
  ../../vendor/gf-imgui/imgui_impl_gf.cc
  ../../vendor/gf-imgui/imgui/imgui.cpp
  ../../vendor/gf-imgui/imgui/imgui_demo.cpp
  ../../vendor/gf-imgui/imgui/imgui_draw.cpp
  ../../vendor/gf-imgui/imgui/imgui_widgets.cpp
)

target_compile_features(gf_dungeons
  PUBLIC
    cxx_std_14
)

set_target_properties(gf_dungeons
  PROPERTIES
    CXX_EXTENSIONS OFF
)

target_include_directories(gf_dungeons
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/gf-imgui/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/gf-imgui"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(gf_dungeons
  PRIVATE
    gf::graphics
)

install(
  TARGETS gf_dungeons
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
