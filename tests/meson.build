cheese_tests = [
  'cheese-test-camera',
  'cheese-test-chooser',
  'cheese-test-flash',
  'cheese-test-monitor',
  'cheese-test-widget',
]

foreach cheese_test: cheese_tests
  executable(
    cheese_test,
    sources: cheese_test + '.c',
    include_directories: top_inc,
    dependencies: libcheese_gtk_dep,
  )
endforeach

random_number = cheese_minor_version + meson.version().split('.').get(1).to_int()

test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())
test_env.set('G_DEBUG', 'gc-friendly')
test_env.set('MALLOC_CHECK_', '2')
test_env.set('MALLOC_PERTURB_', '@0@'.format(random_number % 256))

unit_tests = [
  ['test-libcheese', [], libcheese_dep],
  ['test-libcheese-gtk', um_crop_area_source, libcheese_gtk_dep],
]

if have_xtest
  unit_tests += [['test-webcam-button', [], [x11_dep, xtst_dep]]]
endif

foreach unit_test: unit_tests
  exe = executable(
    unit_test[0],
    sources: [unit_test[0] + '.c'] + unit_test[1],
    include_directories: top_inc,
    dependencies: unit_test[2],
  )

  test(
    unit_test[0],
    exe,
    env: test_env
  )
endforeach
