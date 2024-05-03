project('platon-gtk', ['c', 'cpp'], default_options: ['b_ndebug=if-release'])

executable(
	meson.project_name(),
	'core/prism/prism.cpp',
	'application.c',
	'window.c',
	'editor_widget.cpp',
	dependencies: [
		dependency('gtk+-3.0'),
	],
	override_options: ['cpp_std=c++17']
)
