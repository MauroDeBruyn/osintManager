# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\OSINT_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\OSINT_autogen.dir\\ParseCache.txt"
  "OSINT_autogen"
  )
endif()
