#include "mutekix/assets.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <muteki/fs.h>

// Brief overview of the PKZIP format: https://users.cs.jmu.edu/buchhofp/forensics/formats/pkzip.html
// Official PKZIP format specification: https://pkwaredownloads.blob.core.windows.net/pem/APPNOTE.txt

#define PKZIP_CDR 0x02014b50u
#define PKZIP_LFH 0x04034b50u
#define PKZIP_ECDR 0x06054b50u

typedef struct pkzip_lfh_s {
    uint16_t version;
    uint16_t flags;
    uint16_t compression;
    uint16_t mtime;
    uint16_t mdate;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_size;
    uint16_t extra_fields_size;
} __attribute__((packed)) pkzip_lfh_t;

typedef struct pkzip_cdr_s {
    uint16_t spec_version;
    uint16_t version;
    uint16_t flags;
    uint16_t compression;
    uint16_t mtime;
    uint16_t mdate;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_size;
    uint16_t extra_fields_size;
    uint16_t comment_size;
    uint16_t vol_index;
    uint16_t internal_attrs;
    uint32_t external_attrs;
    uint32_t lfh_offset;
} __attribute__((packed)) pkzip_cdr_t;

typedef struct pkzip_ecdr_s {
    uint16_t vol_index;
    uint16_t first_cdr_vol_index;
    uint16_t cdr_count;
    uint16_t cdr_total_entries;
    uint32_t cdr_total_size;
    uint32_t first_cdr_offset;
    uint16_t comment_size;
} __attribute__((packed)) pkzip_ecdr_t;

static bool mutekix_assets_create_index(mutekix_assets_index_dict_t index_root, loader_file_descriptor_t *ldrfd) {
    bool result = true;

    loader_file_descriptor_t *bundle_file = _OpenSubFile(ldrfd, 0, _FileSize(ldrfd));

    if (bundle_file == NULL) {
        WriteComDebugMsg("mutekix_assets_create_index: Failed to duplicate asset bundle file descriptor.\n");
        return NULL;
    }

    mutekix_assets_index_dict_init(index_root);

    while (true) {
        uint32_t magic;
        bool abort_parsing = false;
        if (_ReadFile(bundle_file, &magic, sizeof(magic)) != sizeof(magic)) {
            break;
        }
        switch (magic) {
            case PKZIP_CDR: {
                pkzip_cdr_t cdr;
                if (_ReadFile(bundle_file, &cdr, sizeof(cdr)) != sizeof(cdr)) {
                    WriteComDebugMsg("mutekix_assets_create_index: Early EOF when reading central directory record\n");
                    abort_parsing = true;
                    break;
                }
                _FseekFile(bundle_file, cdr.filename_size + cdr.extra_fields_size + cdr.comment_size, _SYS_SEEK_CUR);
                break;
            }
            case PKZIP_LFH: {
                pkzip_lfh_t lfh;
                if (_ReadFile(bundle_file, &lfh, sizeof(lfh)) != sizeof(lfh)) {
                    WriteComDebugMsg("mutekix_assets_create_index: Early EOF when reading local file header\n");
                    abort_parsing = true;
                    break;
                }
                if (lfh.compression != 0) {
                    WriteComDebugMsg("mutekix_assets_create_index: Compressed ZIP archive is not supported\n");
                    abort_parsing = true;
                    break;
                }

                size_t extra_size = lfh.filename_size + lfh.extra_fields_size;
                ssize_t tell_result = _TellFile(bundle_file);
                if (tell_result < 0) {
                    WriteComDebugMsg("mutekix_assets_create_index: _TellFile() failed\n");
                    abort_parsing = true;
                    break;
                }

                char *path_c_str = calloc(lfh.filename_size + sizeof(char), sizeof(char));
                if (path_c_str == NULL) {
                    WriteComDebugMsg("mutekix_assets_create_index: Unable to allocate buffer for temporary filename storage\n");
                    abort_parsing = true;
                    break;
                }

                if (_ReadFile(bundle_file, path_c_str, lfh.filename_size) != lfh.filename_size) {
                    WriteComDebugMsg("mutekix_assets_create_index: Early EOF when skipping file\n");
                    free(path_c_str);
                    abort_parsing = true;
                    break;
                }
                // Skip to next file
                _FseekFile(bundle_file, lfh.extra_fields_size + lfh.compressed_size, _SYS_SEEK_CUR);

                string_t path;
                string_init_set_str(path, path_c_str);
                free(path_c_str);
                path_c_str = NULL;

                if (string_end_with_str_p(path, "/")) {
                    WriteComDebugMsg("mutekix_assets_create_index: Skip directory '%s'\n", string_get_cstr(path));
                    string_clear(path);
                    break;
                }

                mutekix_assets_index_dict_entry_t data_entry = {
                    .offset = ((size_t) (tell_result & 0x7fffffff)) + extra_size,
                    .size = lfh.compressed_size
                };

                mutekix_assets_index_dict_set_at(index_root, path, data_entry);
                string_clear(path);
                break;
            }
            case PKZIP_ECDR: {
                pkzip_ecdr_t ecdr;
                if (_ReadFile(bundle_file, &ecdr, sizeof(ecdr)) != sizeof(ecdr)) {
                    WriteComDebugMsg("mutekix_assets_create_index: Early EOF when reading end of central directory record\n");
                    abort_parsing = true;
                    break;
                }
                _FseekFile(bundle_file, ecdr.comment_size, _SYS_SEEK_CUR);
                break;
            }
            default: {
                WriteComDebugMsg("mutekix_assets_create_index: Unknown PKZIP header 0x%08lx\n", magic);
                abort_parsing = true;
            }
        }
        if (abort_parsing) {
            _CloseFile(bundle_file);
            mutekix_assets_index_dict_clear(index_root);
            result = false;
            break;
        }
    }
    _CloseFile(bundle_file);
    return result;
}

loader_file_descriptor_t *mutekix_assets_get_root() {
    loader_loaded_t *loaded = GetApplicationProcW(GetCurrentPathW());
    loader_file_descriptor_t *root = NULL;

    if (loaded == NULL) {
        WriteComDebugMsg("mutekix_assets_init_applet: Failed to get current applet instance.\n");
        return false;
    }
    if (loaded->asset_file != NULL) {
        return _OpenSubFile(loaded->asset_file, 0, _FileSize(loaded->asset_file));
    }

    if (loaded->ldrfd == NULL) {
        WriteComDebugMsg("mutekix_assets_init_applet: ldrfd is null.");
        return NULL;
    }

    size_t argv0_len = loaded->path_lfn == NULL ? 0 : wcslen((UTF16 *) loaded->path_lfn);
    if (argv0_len == 0) {
        WriteComDebugMsg("mutekix_assets_get_root: argv0 is null or an empty string.\n");
        return NULL;
    }

    fs_parts_lfn_t *argv0_parts = calloc(1, sizeof(fs_parts_lfn_t));
    if (argv0_parts == NULL) {
        WriteComDebugMsg("mutekix_assets_get_root: Cannot allocate memory.\n");
        return NULL;
    }
    wcsncpy(argv0_parts->pathname, loaded->path_lfn, sizeof(argv0_parts->pathname) / sizeof(argv0_parts->pathname[0]));
    if (_wfnsplit(argv0_parts->pathname, argv0_parts->drive, argv0_parts->dirname, argv0_parts->basename, argv0_parts->suffix) < 0) {
        WriteComDebugMsg("mutekix_assets_get_root: Failed to parse argv0.\n");
        free(argv0_parts);
        return NULL;
    };
    wcsncpy(argv0_parts->suffix, _BUL(".dat"), sizeof(argv0_parts->suffix) / sizeof(argv0_parts->suffix[0]));
    if (_wfnmerge(argv0_parts->pathname, argv0_parts->drive, argv0_parts->dirname, argv0_parts->basename, argv0_parts->suffix) < 0) {
        WriteComDebugMsg("mutekix_assets_get_root: Failed to build asset bundle path.\n");
        free(argv0_parts);
        return NULL;
    }
    // TODO should we prefer internal asset bundle over external?
    root = _OpenFileW(argv0_parts->pathname, _BUL("rb"));
    free(argv0_parts);
    if (root != NULL) {
        return root;
    }

    loader_applet_info_t info = {0};
    GetApplicationHeadInfoW(loaded->path_lfn, &info);

    ssize_t executable_size = _FileSize(loaded->ldrfd);
    if (executable_size < 0) {
        WriteComDebugMsg("mutekix_assets_get_root: Failed to get executable file size.\n");
        return NULL;
    }
    if (((size_t) (executable_size & 0x7fffffff)) <= info.exe_raw_size) {
        WriteComDebugMsg("mutekix_assets_get_root: No asset bundle found.\n");
        return NULL;
    }

    root = _OpenSubFile(loaded->ldrfd, info.exe_raw_size, info.exe_raw_size - ((size_t) (executable_size & 0x7fffffff)));

    return root;
}

bool mutekix_assets_init_applet(mutekix_assets_t *ctx) {
    loader_file_descriptor_t *root = mutekix_assets_get_root();
    if (root == NULL || _FileSize(root) <= 0) {
        WriteComDebugMsg("mutekix_assets_init_applet: Applet does not have an assets bundle or it's an empty file.\n");
        return false;
    }
    ctx->root = root;
    bool index_created = mutekix_assets_create_index(ctx->index, root);
    if (index_created == false) {
        _CloseFile(ctx->root);
        ctx->root = NULL;
        WriteComDebugMsg("mutekix_assets_init_applet: Assets bundle is of unknown format.\n");
        return false;
    }
    return true;
}

void mutekix_assets_fini(mutekix_assets_t *ctx) {
    if (ctx == NULL || ctx->root == NULL) {
        return;
    }
    mutekix_assets_index_dict_clear(ctx->index);
    _CloseFile(ctx->root);
    ctx->root = NULL;
}

loader_file_descriptor_t *mutekix_assets_open(mutekix_assets_t *ctx, const char *path) {
    if (ctx == NULL || ctx->root == NULL) {
        return NULL;
    }
    string_t path_mstr;
    string_init_set_str(path_mstr, path);
    mutekix_assets_index_dict_entry_t *entry = mutekix_assets_index_dict_get(ctx->index, path_mstr);
    string_clear(path_mstr);

    if (entry == NULL) {
        return NULL;
    }

    return _OpenSubFile(ctx->root, entry->offset, entry->size);
}
