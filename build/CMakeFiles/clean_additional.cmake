# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\raw-processor_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\raw-processor_autogen.dir\\ParseCache.txt"
  "raw-processor_autogen"
  )
endif()
