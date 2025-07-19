project('mutekix', 'c',
    version : '1.0.0',
    default_options : ['warning_level=3'])

includes = include_directories(['include/', 'ext/mlib/'])
sources = [
    'src/assets.c',
    'src/console.c',
    'src/threading.c',
    'src/time.c',
    'src/thread_wrapper_armv4T.s',
]

headers_mlibjson = [
    'ext/mjsonlib/mjsonlib.h',
]

sources_mlibjson = [
    'ext/mjsonlib/mjsonlib.c',
]

mlib_sample_dirs = [
    'doc',
    'example',
    'tests',
    '.github',
]

mlib_misc_files = [
    'Makefile',
    'LICENSE',
    '.gitignore',
    '.git',
    'README.md',
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
    [sources, sources_mlibjson],
    include_directories : includes,
    install : true,
    c_args : c_args,
    pic : false,
)

install_subdir(
    'include/',
    install_dir : 'include/',
    exclude_directories : mlib_sample_dirs,
    exclude_files : mlib_misc_files,
    strip_directory : true,
)

install_subdir(
    'ext/mlib/',
    install_dir : 'include/mutekix/mlib/',
    exclude_directories : mlib_sample_dirs,
    exclude_files : mlib_misc_files,
    strip_directory : true,
)

install_headers(
    headers_mlibjson,
    install_dir : 'include/mutekix/mlibjson/',
    preserve_path : false,
)
