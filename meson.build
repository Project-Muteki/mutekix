project('mutekix', 'c',
    version : '1.0.0',
    default_options : ['warning_level=3'])

sources = [
    'src/console.c',
]

static_library(
    'mutekix',
    sources,
    include_directories : ['include/'],
    install : true,
    c_args : ['-mcpu=arm7tdmi', '-Wall'],
    pic : false,
)

install_subdir(
    'include/',
    install_dir : 'include/',
    strip_directory : true,
)
