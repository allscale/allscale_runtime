
set(subdirs traits core util)

foreach(subdir ${subdirs})
  add_subdirectory(${subdir})
endforeach()

