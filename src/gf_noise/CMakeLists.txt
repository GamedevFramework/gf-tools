configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

add_executable(gf_noise
  gf_noise.cc

  bits/NoiseApp.cc
  bits/NoiseDisplay.cc
  bits/NoiseGui.cc
  bits/NoiseScene.cc

  ../../vendor/gf-imgui/imgui_impl_gf.cc
  ../../vendor/gf-imgui/imgui/imgui.cpp
  ../../vendor/gf-imgui/imgui/imgui_demo.cpp
  ../../vendor/gf-imgui/imgui/imgui_draw.cpp
  ../../vendor/gf-imgui/imgui/imgui_widgets.cpp
)

target_compile_features(gf_noise
  PUBLIC
    cxx_std_14
)

set_target_properties(gf_noise
  PROPERTIES
    CXX_EXTENSIONS OFF
)

target_include_directories(gf_noise
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/gf-imgui/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/gf-imgui"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(gf_noise
  PRIVATE
    gf::gf0
)

install(
  TARGETS gf_noise
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
