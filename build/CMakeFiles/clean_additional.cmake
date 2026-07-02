# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\raw-processor_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\raw-processor_autogen.dir\\ParseCache.txt"
  "libraw\\raw_autogen"
  "libraw\\raw_r_autogen"
  "raw-processor_autogen"
  )
endif()
