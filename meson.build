project('mutekix', 'c',
    version : '1.0.0',
    default_options : ['warning_level=3'])

sources = [
    'src/console.c',
    'src/threading.c',
    'src/thread_wrapper_armv4T.s',
]

c_args = ['-mcpu=arm7tdmi', '-Wall']

if get_option('provide_tls')
    c_args += '-DMUTEKIX_PROVIDE_TLS'
endif

if get_option('use_only_public_interfaces')
    c_args += '-DMUTEKIX_USE_ONLY_PUBLIC_INTERFACES'
endif

static_library(
    'mutekix',
    sources,
    include_directories : ['include/'],
    install : true,
    c_args : c_args,
    pic : false,
)

install_subdir(
    'include/',
    install_dir : 'include/',
    strip_directory : true,
)
