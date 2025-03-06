project('mutekix', 'c',
    version : '1.0.0',
    default_options : ['warning_level=3'])

includes = include_directories(['include/'])
sources = [
    'src/assets.c',
    'src/console.c',
    'src/threading.c',
    'src/thread_wrapper_armv4T.s',
]

mlib_sample_dirs = [
    'mutekix/mlib/doc',
    'mutekix/mlib/example',
    'mutekix/mlib/tests',
    'mutekix/mlib/.github',
]

mlib_misc_files = [
    'mutekix/mlib/Makefile',
    'mutekix/mlib/LICENSE',
    'mutekix/mlib/.gitignore',
    'mutekix/mlib/.git',
    'mutekix/mlib/README.md',
]

c_args = []

if get_option('provide_tls')
    c_args += '-DMUTEKIX_PROVIDE_TLS'
endif

if get_option('use_only_public_interfaces')
    c_args += '-DMUTEKIX_USE_ONLY_PUBLIC_INTERFACES'
endif

static_library(
    'mutekix',
    sources,
    include_directories : includes,
    install : true,
    c_args : c_args,
    pic : false,
)

install_subdir(
    'include/',
    install_dir : 'include/',
    exclude_directories: mlib_sample_dirs,
    exclude_files: mlib_misc_files,
    strip_directory : true,
)
