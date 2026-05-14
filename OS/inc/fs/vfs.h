#pragma once
#include <stddef.h>
#include <stdint.h>

/*
 * vfs_list_cb_t - callback used when listing directory contents
 * @name: node name
 * @is_dir: nonzero if node is a directory
 * @user: caller-provided context pointer
 */
typedef void (*vfs_list_cb_t)(const char* name, int is_dir, void* user);

/*
 * vfs_status_t - VFS operation status codes
 */
typedef enum {
    VFS_OK = 0,
    VFS_ERR_NOT_FOUND,
    VFS_ERR_EXISTS,
    VFS_ERR_NOT_DIR,
    VFS_ERR_IS_DIR,
    VFS_ERR_NAME_INVALID,
    VFS_ERR_NO_MEM,
    VFS_ERR_BUSY
} vfs_status_t;

typedef enum {
    VFS_WARP_FAIL = 0,
    VFS_WARP_OVERWRITE,
    VFS_WARP_RENAME
} vfs_warp_mode_t;

/**
 * vfs_init - initialize the in-memory virtual filesystem
 *
 * Creates the default root structure and sets the working directory.
 */
void vfs_init(void);

/**
 * vfs_pwd - get current working directory path
 * @out: output buffer
 * @cap: output buffer capacity
 */
void vfs_pwd(char* out, size_t cap);

/**
 * vfs_cd - change current working directory
 * @name: target path
 *
 * Return: VFS status code
 */
vfs_status_t vfs_cd(const char* name);

/**
 * vfs_mkdir - create a directory in the current directory
 * @name: directory name
 */
vfs_status_t vfs_mkdir(const char* name);

/**
 * vfs_fab - create an empty file
 * @filename: file name
 */
vfs_status_t vfs_fab(const char* filename);

/**
 * vfs_insp - inspect/read a text file
 * @filename: file name
 * @out_text: returned file contents
 */
vfs_status_t vfs_insp(const char* filename, const char** out_text);

/**
 * vfs_carve - write text into a file
 * @filename: target file
 * @text: null-terminated text buffer
 */
vfs_status_t vfs_carve(const char* filename, const char* text);

/**
 * vfs_insp_bytes - inspect/read binary file contents
 * @filename: file name
 * @out_data: returned data pointer
 * @out_size: returned byte count
 */
vfs_status_t vfs_insp_bytes(const char* filename, const uint8_t** out_data, size_t* out_size);

/**
 * vfs_carve_bytes - write binary data into a file
 * @filename: target file
 * @data: source byte buffer
 * @size: byte count
 */
vfs_status_t vfs_carve_bytes(const char* filename, const uint8_t* data, size_t size);

/**
 * vfs_burn - delete a file or empty directory
 * @filename: target node name
 */
vfs_status_t vfs_burn(const char* filename);

/**
 * vfs_load - load filesystem image from disk
 */
vfs_status_t vfs_load(void);

/**
 * vfs_save - save filesystem image to disk
 */
vfs_status_t vfs_save(void);

/**
 * vfs_learn - mark a script as executable (mark spell as learned)
 * @filename: .ms script filename
 */
vfs_status_t vfs_learn(const char* filename);

/**
 * vfs_is_learned - check whether a script is executable (spell is learned)
 * @filename: .ms script filename
 * @out_learned: returned learn state
 */
vfs_status_t vfs_is_learned(const char* filename, int* out_learned);

/**
 * vfs_shop - enumerate directory contents
 * @cb: callback for each node
 * @user: caller context pointer
 */
void vfs_shop(vfs_list_cb_t cb, void* user);

/**
 * vfs_is_dirty - check whether filesystem has unsaved changes
 */
int vfs_is_dirty(void);

/**
 * vfs_warp - move or rename a file/directory
 * @src_name: source node name
 * @dest_path: destination directory path
 * @mode: conflict resolution mode
 * @out_final_name: returned final node name
 * @out_final_name_cap: output buffer capacity
 */
vfs_status_t vfs_warp(const char* src_name, const char* dest_path, vfs_warp_mode_t mode, char* out_final_name, size_t out_final_name_cap);

typedef void (*vfs_spell_cb_t)(const char* name, void* user);

/**
 * vfs_grimoire - enumerate learned spell scripts
 * @cb: callback for each learned spell
 * @user: caller context pointer
 */
void vfs_grimoire(vfs_spell_cb_t cb, void* user);
