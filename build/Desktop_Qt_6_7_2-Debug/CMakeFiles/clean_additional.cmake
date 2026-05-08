# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/FileExplorer_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/FileExplorer_autogen.dir/ParseCache.txt"
  "FileExplorer_autogen"
  )
endif()
