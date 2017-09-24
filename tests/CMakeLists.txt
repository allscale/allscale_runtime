set(subdirs "")

set(subdirs ${subdirs} unit regressions performance)
#set(subdirs ${subdirs} unit )

foreach(subdir ${subdirs})
  add_subdirectory(${subdir})
endforeach()

