# FindReadline.cmake
# 查找 Readline 库
# 定义以下变量:
#  READLINE_FOUND - 系统中是否有 Readline
#  READLINE_INCLUDE_DIR - Readline 头文件目录
#  READLINE_LIBRARIES - 链接 Readline 需要的库

find_path(READLINE_INCLUDE_DIR readline/readline.h
  /usr/include
  /usr/local/include
  /opt/local/include
  /sw/include
)

find_library(READLINE_LIBRARY
  NAMES readline
  PATHS /usr/lib /usr/local/lib /opt/local/lib /sw/lib
)

find_library(NCURSES_LIBRARY
  NAMES ncurses
  PATHS /usr/lib /usr/local/lib /opt/local/lib /sw/lib
)

set(READLINE_LIBRARIES ${READLINE_LIBRARY} ${NCURSES_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Readline DEFAULT_MSG READLINE_LIBRARY READLINE_INCLUDE_DIR)

mark_as_advanced(READLINE_INCLUDE_DIR READLINE_LIBRARY NCURSES_LIBRARY) 