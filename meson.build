project('platon-gtk', ['vala', 'c', 'cpp'])

executable(
	meson.project_name(),
	'core/c_api.cpp',
	'platon.vapi',
	'src/Application.vala',
	'src/EditView.vala',
	'src/Line.vala',
	'src/Theme.vala',
	'src/Window.vala',
	dependencies: [
		dependency('gtk+-3.0'),
		dependency('json-glib-1.0'),
	],
	include_directories: include_directories('core'),
	override_options: ['cpp_std=c++17']
)
