
set(subdirs traits core)

foreach(subdir ${subdirs})
  add_subdirectory(${subdir})
endforeach()

