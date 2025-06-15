#include "aux/cJSON.c"
#include <asm-generic/errno-base.h>
#include <cjson/cJSON.h>
#include <fuse3/fuse_opt.h>
#include <glib.h>
#include <stdlib.h>
#define FUSE_USE_VERSION 31

#include <assert.h>
#include <fcntl.h>
#include <fuse.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static struct options {
    const char *structure;
    cJSON *json_structure;
    int show_help;
} options;

#define OPTION(t, p) {t, offsetof(struct options, p), 1}

static const struct fuse_opt option_spec[] = {
    OPTION("--structure=%s", structure), OPTION("-h", show_help),
    OPTION("--help", show_help), FUSE_OPT_END};

static int json_getattr(const char *path, struct stat *stbuf,
                        struct fuse_file_info *fi) {
    (void)fi;
    int res = 0;
    cJSON *item;
    gchar **path_parts = g_strsplit(path + 1, "/", -1);

    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if ((item = cJSON_GetInObjectItemCaseSensitive(
                    options.json_structure, path_parts))) {
        cJSON_bool isObject = cJSON_IsObject(item);
        stbuf->st_mode = isObject ? S_IFDIR | 0755 : S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = isObject ? 0 : strlen(cJSON_GetStringValue(item));
    } else
        res = -ENOENT;

    g_strfreev(path_parts);

    return res;
}

static int json_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi,
                        enum fuse_readdir_flags flags) {
    (void)offset;
    (void)fi;
    (void)flags;

    gchar **path_parts = g_strsplit(path + 1, "/", -1);
    cJSON *field;
    cJSON *item =
        cJSON_GetInObjectItemCaseSensitive(options.json_structure, path_parts);

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    if (!item || !cJSON_IsObject(item)) {
        g_strfreev(path_parts);
        return -ENOENT;
    }

    cJSON_ArrayForEach(field, item) { filler(buf, field->string, NULL, 0, 0); }

    g_strfreev(path_parts);

    return 0;
}

static int json_open(const char *path, struct fuse_file_info *fi) {

    gchar **path_parts = g_strsplit(path + 1, "/", -1);
    cJSON *item =
        cJSON_GetInObjectItemCaseSensitive(options.json_structure, path_parts);

    if (item == NULL)
        return -ENOENT;

    g_strfreev(path_parts);

    return 0;
}

static int json_read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi) {

    (void)fi;
    gchar **path_parts = g_strsplit(path + 1, "/", -1);
    char *item_string = cJSON_GetStringValue(
        cJSON_GetInObjectItemCaseSensitive(options.json_structure, path_parts));
    size_t len = strlen(item_string);

    if (item_string == NULL)
        return -ENOENT;

    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, item_string + offset, size);
    } else
        size = 0;

    g_strfreev(path_parts);

    return size;
}

// The root dorectory case scenario does not run through the functions below

static int json_write(const char *path, const char *buf, size_t size,
                      off_t offset, struct fuse_file_info *fi) {
    (void)fi;
    gchar **path_parts = g_strsplit(path + 1, "/", -1);
    cJSON *item =
        cJSON_GetInObjectItemCaseSensitive(options.json_structure, path_parts);

    if (!item) {
        g_strfreev(path_parts);
        return -ENOENT;
    }

    if (!cJSON_IsString(item)) {
        g_strfreev(path_parts);
        return -EISDIR; // Cannot write to a directory
    }

    size_t actual_size = strlen(buf);
    size_t size_to_write = size < actual_size ? size : actual_size;

    size_t item_length = strlen(cJSON_GetStringValue(item));

    if (offset > item_length) {
        g_strfreev(path_parts);
        return -EINVAL; // Offset is beyond the current length of the item
    }

    // Ensure the item is large enough to hold the new data
    char *new_string = malloc(offset + size_to_write + 1);
    if (!new_string) {
        g_strfreev(path_parts);
        return -ENOMEM;
    }

    // Copy existing data up to offset
    strncpy(new_string, cJSON_GetStringValue(item), offset);
    // Write new data
    memcpy(new_string + offset, buf, size_to_write);
    new_string[offset + size_to_write] = '\0';

    // Update the JSON item
    cJSON_SetValuestring(item, new_string);

    free(new_string);
    g_strfreev(path_parts);

    return size_to_write;
}

static int json_rename(const char *from, const char *to, unsigned int flags) {
    (void)flags;
    gchar **from_parts = g_strsplit(from + 1, "/", -1);
    gchar **to_parts = g_strsplit(to + 1, "/", -1);

    cJSON *from_parent_item = cJSON_GetInObjectItemParentCaseSensitive(
        options.json_structure, from_parts);
    cJSON *to_parent_item = cJSON_GetInObjectItemParentCaseSensitive(
        options.json_structure, to_parts);

    if (!from_parent_item || !to_parent_item) {
        g_strfreev(from_parts);
        g_strfreev(to_parts);
        return -ENOENT;
    }

    gchar *from_last_part = from_parts[g_strv_length(from_parts) - 1];
    cJSON *item =
        cJSON_GetObjectItemCaseSensitive(from_parent_item, from_last_part);

    if (!item) {
        g_strfreev(from_parts);
        g_strfreev(to_parts);
        return -ENOENT;
    }

    gchar *to_last_part = to_parts[g_strv_length(to_parts) - 1];

    if (cJSON_GetObjectItemCaseSensitive(to_parent_item, to_last_part)) {
        g_strfreev(from_parts);
        g_strfreev(to_parts);
        return -EEXIST;
    }

    cJSON_AddItemToObject(to_parent_item, to_last_part,
                          cJSON_Duplicate(item, TRUE));
    cJSON_DeleteItemFromObjectCaseSensitive(from_parent_item, from_last_part);

    g_strfreev(from_parts);
    g_strfreev(to_parts);

    return 0;
}

static int json_unlink(const char *path) {
    gchar **path_parts = g_strsplit(path + 1, "/", -1);
    gchar *last_part = path_parts[g_strv_length(path_parts) - 1];
    cJSON *parent_item = cJSON_GetInObjectItemParentCaseSensitive(
        options.json_structure, path_parts);

    if (!parent_item) {
        g_strfreev(path_parts);
        return -ENOENT;
    }

    cJSON *item = cJSON_GetObjectItemCaseSensitive(parent_item, last_part);

    if (!item) {
        g_strfreev(path_parts);
        return -ENOENT;
    }

    cJSON_DeleteItemFromObjectCaseSensitive(parent_item, last_part);

    g_strfreev(path_parts);

    return 0;
}

static int json_rmdir(const char *path) {
    gchar **path_parts = g_strsplit(path + 1, "/", -1);
    gchar *last_part = path_parts[g_strv_length(path_parts) - 1];
    cJSON *parent_item = cJSON_GetInObjectItemParentCaseSensitive(
        options.json_structure, path_parts);

    if (!parent_item) {
        g_strfreev(path_parts);
        return -ENOENT;
    }

    cJSON *item = cJSON_GetObjectItemCaseSensitive(parent_item, last_part);

    if (!item || !cJSON_IsObject(item)) {
        g_strfreev(path_parts);
        return -ENOENT;
    }

    if (cJSON_GetArraySize(item) > 0) {
        g_strfreev(path_parts);
        return -ENOTEMPTY;
    }

    cJSON_DeleteItemFromObjectCaseSensitive(parent_item, last_part);

    g_strfreev(path_parts);

    return 0;
}

static int create(const char *path, cJSON *(*create_function)()) {
    gchar **path_parts = g_strsplit(path + 1, "/", -1);
    gchar *last_part = path_parts[g_strv_length(path_parts) - 1];
    cJSON *parent_item = cJSON_GetInObjectItemParentCaseSensitive(
        options.json_structure, path_parts);

    if (!parent_item) {
        g_strfreev(path_parts);
        return -ENOENT;
    }

    if (cJSON_GetObjectItemCaseSensitive(parent_item, last_part)) {
        g_strfreev(path_parts);
        return -EEXIST;
    }

    cJSON_AddItemToObject(parent_item, last_part, create_function());

    g_strfreev(path_parts);

    return 0;
}

static int json_mkdir(const char *path, mode_t mode) {
    (void)mode;
    return create(path, cJSON_CreateObject);
}

static int json_create(const char *path, mode_t mode,
                       struct fuse_file_info *fi) {
    (void)mode;
    return create(path, cJSON_CreateEmptyString);
}

static int json_utimens(const char *path, const struct timespec tv[2],
                        struct fuse_file_info *fi) {
    (void)tv;
    (void)fi;
    (void)path;

    // This file-system does not support timestamps
    return 0;
}

static const struct fuse_operations json_oper = {
    .getattr = json_getattr,
    .readdir = json_readdir,
    .open = json_open,
    .read = json_read,
    .write = json_write,
    .rename = json_rename,
    .unlink = json_unlink,
    .rmdir = json_rmdir,
    .mkdir = json_mkdir,
    .create = json_create,
    .utimens = json_utimens,
};

static void show_help(const char *progname) {
    printf("usage: %s [options] <mountpoint>\n\n", progname);
    printf(
        "File-system specific options:\n"
        "    --structure=<json>  JSON structure to return for the filesystem\n"
        "\n");
}

int main(int argc, char *argv[]) {
    int ret;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    /* Set defaults -- we have to use strdup so that
       fuse_opt_parse can free the defaults if other
       values are specified */
    options.structure =
        strdup("{\"hella\":\"Hello World!\n\",\"foo\": {\"bar\": \"baz\n\"}}");

    /* Parse options */
    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
        return 1;

    /* Parse JSON structure */
    options.json_structure = cJSON_Parse(options.structure);
    if (options.json_structure == NULL) {
        fprintf(stderr, "Failed to parse JSON structure: %s\n",
                cJSON_GetErrorPtr());
        return 1;
    }

    /* When --help is specified, first print our own file-system
       specific help text, then signal fuse_main to show
       additional help (by adding `--help` to the options again)
       without usage: line (by setting argv[0] to the empty
       string) */
    if (options.show_help) {
        show_help(argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    }

    ret = fuse_main(args.argc, args.argv, &json_oper, NULL);
    fuse_opt_free_args(&args);
    return ret;
}
