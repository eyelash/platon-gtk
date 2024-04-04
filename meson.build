project('platon-gtk', ['c', 'cpp'], default_options: ['b_ndebug=if-release'])

executable(
	meson.project_name(),
	'core/c_api.cpp',
	'core/prism/prism.cpp',
	'application.c',
	'window.c',
	dependencies: [
		dependency('gtk+-3.0'),
	],
	include_directories: include_directories('core'),
	override_options: ['cpp_std=c++17']
)
