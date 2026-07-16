cmake_minimum_required(VERSION 3.16)

# The ST7305 and codec ports are the unmodified Waveshare components shipped in
# this repository. Keeping them as explicit component roots makes provenance
# auditable and avoids a second, drifting copy of board-specific code.
set(WAVESHARE_EXAMPLES "${CMAKE_CURRENT_LIST_DIR}/../waveshare-official/02_Example/ESP-IDF")
set(EXTRA_COMPONENT_DIRS
    "${WAVESHARE_EXAMPLES}/11_U8G2_Test/components/u8g2"
    "${WAVESHARE_EXAMPLES}/11_U8G2_Test/components/u8g2_st7305"
    "${WAVESHARE_EXAMPLES}/07_Audio_Test/components/ExternLib/codec_board"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(neko_assistant VERSION 0.1.0)
