
set(subdirs traits core util monitor)

foreach(subdir ${subdirs})
  add_subdirectory(${subdir})
endforeach()

