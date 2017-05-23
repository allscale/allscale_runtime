set(subdirs "")

set(subdirs ${subdirs} unit regressions)

foreach(subdir ${subdirs})
  add_subdirectory(${subdir})
endforeach()

