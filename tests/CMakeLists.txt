set(subdirs "")

set(subdirs ${subdirs} unit regressions)
#set(subdirs ${subdirs} unit )

foreach(subdir ${subdirs})
  add_subdirectory(${subdir})
endforeach()

