set(subdirs "")

set(subdirs ${subdirs} unit)

foreach(subdir ${subdirs})
  add_subdirectory(${subdir})
endforeach()

