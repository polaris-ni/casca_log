/**
 * @author Polaris
 * @date 2026/2/12
 */

#include "clog_file_system.h"
#include "clog_error.h"
#include "clog_hooks.h"
#include "clog_platform.h"
#include "clog_secure_func.h"
#ifdef CLOG_PLATFORM_WINDOWS
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct clog_path_node {
    char *name;
    size_t len;
    struct clog_path_node *prev;
    struct clog_path_node *next;
} clog_path_node_t;

#endif

#define CLOG_FILE_CHECK_FLAG(flags, target) (((flags) & (target)) != 0)

struct clog_file {
    uint64_t flags;
#ifdef CLOG_PLATFORM_WINDOWS
    HANDLE handle;
#else
    int fd;
#endif
};

#ifdef CLOG_PLATFORM_WINDOWS
static wchar_t *clog_utf8_to_utf16(const char *utf8_str)
{
    const int len = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
    CLOG_RET_IF(len <= 0, NULL);
    wchar_t *w_str = clog_malloc(len * sizeof(wchar_t));
    CLOG_RET_IF_NULL(w_str, NULL);
    if (MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, w_str, len) <= 0) {
        clog_free(w_str);
        return NULL;
    }
    return w_str;
}
#endif

clog_file_t *clog_file_open(const char *path, uint64_t flags, uint64_t modes)
{
    CLOG_RET_IF_NULL_X(path, NULL, "path is NULL");
    CLOG_RET_IF_X(flags == CLOG_FILE_NONE, NULL, "flags is CLOG_FILE_NONE");
#ifdef CLOG_PLATFORM_WINDOWS
    CLOG_UNUSED_VAR(modes);
    DWORD access = 0;
    DWORD disposition = 0;
    DWORD share = 0;
    DWORD flag = FILE_ATTRIBUTE_NORMAL;
    if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_WRITE)) {
        access |= GENERIC_WRITE;
        if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_CREATE)) {
            if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_EXIST)) {
                disposition = CREATE_NEW;
            } else {
                disposition = CREATE_ALWAYS;
            }
        } else {
            disposition = OPEN_EXISTING;
        }
        flag |= FILE_FLAG_SEQUENTIAL_SCAN;
        if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_SHARED)) {
            share |= FILE_SHARE_DELETE;
            share |= FILE_SHARE_WRITE;
        }
    }
    if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_READ)) {
        access |= GENERIC_READ;
        if (disposition == 0) {
            disposition = OPEN_EXISTING;
        }
        if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_SHARED)) {
            share |= FILE_SHARE_READ;
        }
    }
    if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_DIRECT)) {
        flag |= FILE_FLAG_NO_BUFFERING;
    }
    if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_SYNC)) {
        flag |= FILE_FLAG_WRITE_THROUGH;
    }

    wchar_t *w_path = clog_utf8_to_utf16(path);
    CLOG_RET_IF_NULL(w_path, NULL);

    HANDLE handle = CreateFileW(w_path, access, share, NULL, disposition, flag, NULL);
    CLOG_SAFE_FREE(w_path);
    CLOG_RET_IF_X(handle == INVALID_HANDLE_VALUE, NULL, "CreateFileW failed, flags = 0x%llX err = 0x%llX", flags,
                  (uint64_t)GetLastError());

    if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_WRITE)) {
        const DWORD move = CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_TRUNCATE) ? FILE_BEGIN : FILE_END;
        if (SetFilePointer(handle, 0, NULL, move) == INVALID_SET_FILE_POINTER) {
            CLOG_ERR_ADD("SetFilePointer %u failed, err = 0x%llX", move, (uint64_t)GetLastError());
            CloseHandle(handle);
            return NULL;
        }
        if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_TRUNCATE)) {
            if (!SetEndOfFile(handle)) {
                CLOG_ERR_ADD("SetEndOfFile failed, err = 0x%llX", (uint64_t)GetLastError());
                CloseHandle(handle);
                return NULL;
            }
        }
    }

    clog_file_t *tmp = clog_malloc(sizeof(clog_file_t));
    CLOG_CLEAN_RET_IF_NULL_X(tmp, CloseHandle(handle), NULL, "malloc clog_file_t error");
    tmp->handle = handle;
    tmp->flags = flags;
    handle = INVALID_HANDLE_VALUE;
    return tmp;
#else
    int flag = 0;
    const bool write = CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_WRITE);
    const bool read = CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_READ);
    if (write && read) {
        flag |= O_RDWR;
    } else if (write) {
        flag |= O_WRONLY;
    } else if (read) {
        flag |= O_RDONLY;
    }
    if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_CREATE)) {
        flag |= O_CREAT;
        if (!CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_EXIST)) {
            /* CLOG_FILE_EXIST not specified, if file exists, it will be truncated */
            flag |= O_TRUNC;
        }
    }
    if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_TRUNCATE)) {
        flag |= O_TRUNC;
    } else {
        flag |= O_APPEND;
    }
#ifdef O_DIRECT
    if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_DIRECT)) {
        flag |= O_DIRECT;
    }
#endif
    if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_SYNC)) {
        flag |= O_SYNC;
    }
    const int fd = open(path, flag, (mode_t)modes);
    CLOG_RET_IF_X(fd < 0, NULL, "open failed, err = %d", errno);
    if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_WRITE) && !CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_SHARED)) {
        CLOG_IGNORE_RES(flock(fd, LOCK_EX | LOCK_NB));
    }
    clog_file_t *tmp = clog_malloc(sizeof(clog_file_t));
    CLOG_CLEAN_RET_IF_NULL_X(tmp, close(fd), NULL, "malloc clog_file_t error");
    tmp->fd = fd;
    tmp->flags = flags;
    return tmp;
#endif
}

clog_res_e clog_file_write(const clog_file_t *file, const void *buf, size_t size)
{
    CLOG_RET_IF_NULL_X(file, CLOG_INVALID_PARAM, "file is NULL");
    CLOG_RET_IF_NULL_X(buf, CLOG_INVALID_PARAM, "buf is NULL");
    CLOG_RET_IF_X(size == 0, CLOG_INVALID_PARAM, "buf size is 0");
    CLOG_RET_IF_X(!CLOG_FILE_CHECK_FLAG(file->flags, CLOG_FILE_WRITE), CLOG_NOT_SUPPORTED, "file not opened for write");
#ifdef CLOG_PLATFORM_WINDOWS
    CLOG_RET_IF_X(file->handle == INVALID_HANDLE_VALUE, CLOG_INVALID_PARAM, "file handle is invalid");
    if (!WriteFile(file->handle, buf, size, NULL, NULL)) {
        CLOG_ERR_ADD("WriteFile failed, err = 0x%llX", (uint64_t)GetLastError());
        return CLOG_FAIL;
    }
    if (CLOG_FILE_CHECK_FLAG(file->flags, CLOG_FILE_SYNC)) {
        if (!FlushFileBuffers(file->handle)) {
            CLOG_ERR_ADD("WARN: FlushFileBuffers failed, size = %zu, err = 0x%llX", size, (uint64_t)GetLastError());
        }
    }
    return CLOG_SUCCESS;
#else
    CLOG_RET_IF_X(file->fd < 0, CLOG_INVALID_PARAM, "fd is invalid");
    const ssize_t ret = write(file->fd, buf, size);
    CLOG_RET_IF_X(ret < 0, CLOG_FAIL, "write failed, size = %zu, err = %d", size, errno);
    return CLOG_SUCCESS;
#endif
}

clog_res_e clog_file_read(const clog_file_t *file, void *buf, size_t size, size_t *num)
{
    CLOG_RET_IF_NULL_X(file, CLOG_INVALID_PARAM, "file is NULL");
    CLOG_RET_IF_NULL_X(buf, CLOG_INVALID_PARAM, "buf is NULL");
    CLOG_RET_IF_X(size == 0, CLOG_INVALID_PARAM, "buf size is 0");
    CLOG_RET_IF_X(!CLOG_FILE_CHECK_FLAG(file->flags, CLOG_FILE_READ), CLOG_NOT_SUPPORTED, "file not opened for read");
#ifdef CLOG_PLATFORM_WINDOWS
    CLOG_RET_IF_X(file->handle == INVALID_HANDLE_VALUE, CLOG_INVALID_PARAM, "file handle is invalid");
    DWORD read_size = 0;
    if (!ReadFile(file->handle, buf, size, &read_size, NULL)) {
        CLOG_ERR_ADD("ReadFile failed, size = %zu, err = 0x%llX", size, (uint64_t)GetLastError());
        return CLOG_FAIL;
    }
    if (num != NULL) {
        *num = read_size;
    }
    return CLOG_SUCCESS;
#else
    CLOG_RET_IF_X(file->fd < 0, CLOG_INVALID_PARAM, "fd is invalid");
    const ssize_t ret = read(file->fd, buf, size);
    CLOG_RET_IF_X(ret < 0, CLOG_FAIL, "read failed, size = %zu, err = %d", size, errno);
    if (num != NULL) {
        *num = ret;
    }
    return CLOG_SUCCESS;
#endif
}

clog_res_e clog_file_seek(const clog_file_t *file, clog_file_seek_e whence, int offset, size_t *num)
{
    CLOG_RET_IF_NULL_X(file, CLOG_INVALID_PARAM, "file is NULL");
    CLOG_RET_IF_X(whence > CLOG_FILE_SEEK_END, CLOG_INVALID_PARAM, "whence %u is invalid", whence);
#ifdef CLOG_PLATFORM_WINDOWS
    CLOG_RET_IF_X(file->handle == INVALID_HANDLE_VALUE, CLOG_INVALID_PARAM, "file h0andle is invalid");
    const DWORD move = whence == CLOG_FILE_SEEK_SET ? FILE_BEGIN
        : whence == CLOG_FILE_SEEK_CUR              ? FILE_CURRENT
                                                    : FILE_END;
    const DWORD ret = SetFilePointer(file->handle, 0, NULL, move);
    if (ret == INVALID_SET_FILE_POINTER) {
        const DWORD err = GetLastError();
        CLOG_RET_IF_X(err != NO_ERROR, CLOG_FAIL, "SetFilePointer failed, err = 0x%llX", err);
        if (num != NULL) {
            *num = ret;
        }
        return CLOG_FAIL;
    }
    return CLOG_SUCCESS;
#else
    const int move = whence == CLOG_FILE_SEEK_SET ? SEEK_SET : whence == CLOG_FILE_SEEK_CUR ? SEEK_CUR : SEEK_END;
    const off_t ret = lseek(file->fd, offset, move);
    CLOG_RET_IF_X(ret == -1, CLOG_FAIL, "lseek failed, offset = %d, move = %d, err = %d", offset, move, errno);
    if (num != NULL) {
        *num = ret;
    }
    return CLOG_SUCCESS;
#endif
}

void clog_file_close(clog_file_t **file)
{
    CLOG_RET_VOID_IF_NULL(file);
    CLOG_RET_VOID_IF_NULL(*file);
#ifdef CLOG_PLATFORM_WINDOWS
    CloseHandle((*file)->handle);
#else
    CLOG_IGNORE_RES(close((*file)->fd));
#endif
    clog_free(*file);
    *file = NULL;
}

clog_res_e clog_cwd(char *path, size_t size)
{
    CLOG_RET_IF_NULL_X(path, CLOG_INVALID_PARAM, "path is NULL");
    CLOG_RET_IF_X(size == 0, CLOG_INVALID_PARAM, "path size is 0");
#ifdef CLOG_PLATFORM_WINDOWS
    const DWORD ret = GetCurrentDirectory(size, path);
    if (ret == 0 || ret >= size) {
        CLOG_ERR_ADD("GetCurrentDirectoryW failed, ret = %u, size = %zu, err = 0x%llX", ret, size,
                     (uint64_t)GetLastError());
        return CLOG_FAIL;
    }
#else
    if (getcwd(path, (int)size) == NULL) {
        CLOG_ERR_ADD("getcwd failed, err = %d", errno);
        return CLOG_FAIL;
    }
#endif
    return CLOG_SUCCESS;
}

#ifndef CLOG_PLATFORM_WINDOWS
static clog_res_e clog_create_path_node(clog_path_node_t **node, const char **last, const char **cur)
{
    const char *last_spliter = *last;
    const char *current = *cur;
    clog_path_node_t *current_node = *node;
    if (last_spliter + 1 == current) {
        /* // */
        *last = current;
        *cur = current + 1;
        return CLOG_SUCCESS;
    }
    if (*(last_spliter + 1) == '.') {
        if (last_spliter + 2 == current) {
            /* /./ */
            *last = current;
            *cur = current + 1;
            return CLOG_SUCCESS;
        }
        if (*(last_spliter + 2) == '.' && last_spliter + 3 == current) {
            /* /../ */
            CLOG_RET_IF_NULL_X(current_node->prev, CLOG_ERROR_FORMAT, "pre directory not found");
            clog_path_node_t *pre = current_node->prev;
            CLOG_SAFE_FREE(current_node->name);
            CLOG_SAFE_FREE(current_node);
            pre->next = NULL;
            *node = pre;
            *last = current;
            *cur = current + 1;
            return CLOG_SUCCESS;
        }
    }
    clog_path_node_t *tmp = clog_malloc(sizeof(clog_path_node_t));
    CLOG_RET_IF_NULL_X(tmp, CLOG_NO_MEMORY, "malloc clog_path_node_t failed");
    tmp->name = clog_strndup(last_spliter + 1, current - last_spliter - 1);
    CLOG_CLEAN_RET_IF_NULL_X(tmp->name, clog_free(tmp), CLOG_NO_MEMORY, "clog_strndup path name failed");
    tmp->len = current - last_spliter - 1;
    tmp->prev = current_node;
    tmp->next = NULL;
    current_node->next = tmp;
    *node = tmp;
    *last = current;
    *cur = current + 1;
    return CLOG_SUCCESS;
}

static clog_res_e clog_get_unnormalized_abs_path(const char *path, char **unnormalized)
{
    char *buffer = NULL;
    if (path[0] != '/') {
        char cwd[CLOG_FILEPATH_MAX_SIZE] = {0};
        clog_res_e res = clog_cwd(cwd, sizeof(cwd));
        CLOG_RET_IF_FAILED_X(res, "clog_cwd failed, ret = %u", res);
        const size_t cwd_len = strlen(cwd);
        const size_t original_len = strlen(path);
        const size_t len = cwd_len + original_len + 2; /* + "/" + null */
        buffer = clog_malloc(len);
        CLOG_RET_IF_NULL_X(buffer, CLOG_NO_MEMORY, "malloc absolute path failed, size = %zu", len);
        res = clog_strcpy(buffer, len, cwd);
        CLOG_CLEAN_RET_IF_FAILED_X(res, clog_free(buffer), "clog_strcpy %s failed, ret = %u", cwd, res);
        buffer[cwd_len] = '/';
        res = clog_strcpy(buffer + cwd_len + 1, len - cwd_len - 1, path);
        CLOG_CLEAN_RET_IF_FAILED_X(res, clog_free(buffer), "clog_strcpy %s failed, ret = %u", path, res);
    } else {
        buffer = clog_strdup(path);
        CLOG_RET_IF_NULL_X(buffer, CLOG_NO_MEMORY, "clog_strdup %s failed", path);
    }

    if (buffer[0] != '/') {
        CLOG_ERR_ADD("absolute path must start with '/', current is %s", buffer);
        clog_free(buffer);
        return CLOG_INVALID_PARAM;
    }
    *unnormalized = buffer;
    return CLOG_SUCCESS;
}
#endif

clog_res_e clog_normalize(const char *path, char *buf, size_t size)
{
    CLOG_RET_IF_NULL_X(path, CLOG_INVALID_PARAM, "path is NULL");
    CLOG_RET_IF_NULL_X(buf, CLOG_INVALID_PARAM, "buf is NULL");
    CLOG_RET_IF_X(size < 2, CLOG_INVALID_PARAM, "path size is %zu", size);
#ifdef CLOG_PLATFORM_WINDOWS
    SetLastError(0);
    const DWORD ret = GetFullPathNameA(path, size, buf, NULL);
    if (ret == 0 || ret >= size || GetLastError() != 0) {
        CLOG_ERR_ADD("GetFullPathNameA failed, ret = %u, err = 0x%llX", ret, (uint64_t)GetLastError());
        return CLOG_FAIL;
    }
    const size_t len = strlen(buf);
    if (len > 0 && (buf[len - 1] == '\\' || buf[len - 1] == '/')) {
        if (!(len == 3 && buf[1] == ':' && buf[2] == '\\') && !(len >= 4 && strncmp(buf, "\\\\?\\", 4) == 0)) {
            buf[len - 1] = '\0';
        }
    }
    return CLOG_SUCCESS;
#else
    char *unnormalize_path = NULL;
    clog_res_e res = clog_get_unnormalized_abs_path(path, &unnormalize_path);
    CLOG_RET_IF_FAILED_X(res, "clog_get_unnormalized_abs_path failed, ret = %u", res);

    clog_path_node_t *head_node = clog_malloc(sizeof(clog_path_node_t));
    CLOG_CLEAN_RET_IF_NULL_X(head_node, clog_free(unnormalize_path), CLOG_NO_MEMORY, "malloc path node failed");
    head_node->name = NULL;
    head_node->next = NULL;
    head_node->prev = NULL;

    clog_path_node_t *current_node = head_node;
    const char *last_spliter = unnormalize_path;
    const char *cur_spliter = unnormalize_path + 1;
    while (*cur_spliter != '\0') {
        if (*cur_spliter != '/') {
            cur_spliter++;
            continue;
        }
        res = clog_create_path_node(&current_node, &last_spliter, &cur_spliter);
        if (res != CLOG_SUCCESS) {
            CLOG_ERR_ADD("normalize path %s failed", unnormalize_path);
            goto cleanup;
        }
    }

    if (cur_spliter != last_spliter + 1) {
        /* /a/b/cd */
        res = clog_create_path_node(&current_node, &last_spliter, &cur_spliter);
        if (res != CLOG_SUCCESS) {
            CLOG_ERR_ADD("normalize path %s failed", unnormalize_path);
            goto cleanup;
        }
    }

    buf[0] = '/';
    size_t offset = 1;
    const clog_path_node_t *tmp = head_node->next;
    while (tmp != NULL) {
        res = clog_strcpy(buf + offset, size - 1 - offset, tmp->name);
        if (res != CLOG_SUCCESS) {
            CLOG_ERR_ADD("clog_strcpy path %s failed", tmp->name);
            goto cleanup;
        }
        offset += tmp->len;
        tmp = tmp->next;
        if (tmp != NULL) {
            if (offset + 2 >= size) {
                CLOG_ERR_ADD("buff size is too small");
                res = CLOG_NO_MEMORY;
                goto cleanup;
            }
            buf[offset] = '/';
            offset++;
        }
    }

    buf[offset] = '\0';
    res = CLOG_SUCCESS;
cleanup:
    while (head_node != NULL) {
        clog_path_node_t *node = head_node->next;
        CLOG_SAFE_FREE(head_node->name);
        CLOG_SAFE_FREE(head_node);
        head_node = node;
    }
    clog_free(unnormalize_path);
    return res;
#endif
}

#ifdef CLOG_PLATFORM_WINDOWS
static clog_res_e clog_dir_create_direct_win(const char *path)
{
    clog_res_e ret = CLOG_FAIL;
    const DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryA(path, NULL)) {
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                CLOG_ERR_ADD("CreateDirectoryA %s failed, err = 0x%llX", path, (uint64_t)GetLastError());
                return CLOG_FAIL;
            }
            ret = CLOG_ALREADY_EXISTED;
        } else {
            ret = CLOG_SUCCESS;
        }
    } else if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        CLOG_ERR_ADD("the name of file %s is the same as directory", path);
        return CLOG_BUSY;
    } else {
        ret = CLOG_ALREADY_EXISTED;
    }
    return ret;
}
#endif

clog_res_e clog_dir_create(const char *path, uint64_t flags)
{
    CLOG_RET_IF_NULL_X(path, CLOG_INVALID_PARAM, "path is NULL");
#ifdef CLOG_PLATFORM_WINDOWS
    CLOG_UNUSED_VAR(flags);
    char temp[CLOG_FILEPATH_MAX_SIZE] = {0};
    clog_res_e ret = clog_normalize(path, temp, sizeof(temp));
    CLOG_RET_IF_FAILED_X(ret, "clog_normalize failed, ret = %u", ret);

    char *p = temp;
    /* skip UNC prefix "\\server\share\" */
    if (p[0] == '\\' && p[1] == '\\') {
        for (int slashes = 0; slashes < 2 && *p;) {
            if (*p++ == '\\') {
                slashes++;
            }
        }
    } else if (p[0] != '\0' && p[1] == ':') {
        /* skip driver letter C:\ */
        p += 2;
        if (*p == '\\') {
            p++;
        }
    }

    /* create recursively */
    for (; *p != '\0'; p++) {
        if (*p == '\\' || *p == '/') {
            const char old = *p;
            *p = '\0';
            ret = clog_dir_create_direct_win(temp);
            CLOG_RET_IF_X(ret != CLOG_SUCCESS && ret != CLOG_ALREADY_EXISTED, ret,
                          "clog_dir_create_direct_win %s failed, ret = %u", temp, ret);
            *p = old;
        }
    }
    ret = clog_dir_create_direct_win(temp);
    CLOG_RET_IF_X(ret != CLOG_SUCCESS && ret != CLOG_ALREADY_EXISTED, ret,
                  "clog_dir_create_direct_win %s failed, ret = %u", temp, ret);
    return ret;
#else
    char normalized_path[CLOG_FILEPATH_MAX_SIZE] = {0};
    CLOG_RET_IF_NULL_X(normalized_path, CLOG_NO_MEMORY, "malloc path buf failed, size = %zu", CLOG_FILEPATH_MAX_SIZE);
    clog_res_e res = clog_normalize(path, normalized_path, sizeof(normalized_path));
    CLOG_RET_IF_FAILED_X(res, "clog_normalize failed, ret = %u", res);
    if (normalized_path[0] == '/' && normalized_path[1] == '\0') {
        return CLOG_ALREADY_EXISTED;
    }

    char *p = normalized_path + 1;
    while (*p != '\0') {
        if (*p == '/') {
            *p = '\0';
            struct stat st;
            if (stat(normalized_path, &st) != 0) {
                if (mkdir(normalized_path, flags) != 0) {
                    CLOG_RET_IF_X(errno != EEXIST, CLOG_FAIL, "mkdir %s failed, err = %d", normalized_path, errno);
                    res = CLOG_ALREADY_EXISTED;
                } else {
                    res = CLOG_SUCCESS;
                }
            } else if (!S_ISDIR(st.st_mode)) {
                return CLOG_BUSY;
            } else {
                res = CLOG_ALREADY_EXISTED;
            }
            *p = '/';
        }
        p++;
    }

    struct stat st;
    if (stat(normalized_path, &st) != 0) {
        if (mkdir(normalized_path, flags) != 0) {
            CLOG_RET_IF_X(errno != EEXIST, CLOG_FAIL, "mkdir %s failed, err = %d", normalized_path, errno);
            res = CLOG_ALREADY_EXISTED;
        } else {
            res = CLOG_SUCCESS;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        res = CLOG_BUSY;
    } else {
        res = CLOG_ALREADY_EXISTED;
    }

    return res;
#endif
}

clog_res_e clog_file_get_name(const char *fullpath, char *file, size_t size)
{
    CLOG_RET_IF_NULL_X(fullpath, CLOG_INVALID_PARAM, "fullpath is NULL");
    CLOG_RET_IF_NULL_X(file, CLOG_INVALID_PARAM, "filename buf is NULL");
#ifdef CLOG_PLATFORM_WINDOWS
    const char *p = strrchr(fullpath, '\\');
#else
    const char *p = strrchr(fullpath, '/');
#endif
    CLOG_RET_IF_NULL_X(p, CLOG_INVALID_PARAM, "fullpath is invalid");
    return clog_strcpy(file, size, p + 1);
}

clog_res_e clog_file_get_dir(const char *fullpath, char *dir, size_t size)
{
    CLOG_RET_IF_NULL_X(fullpath, CLOG_INVALID_PARAM, "fullpath is NULL");
    CLOG_RET_IF_NULL_X(dir, CLOG_INVALID_PARAM, "dir buf is NULL");
#ifdef CLOG_PLATFORM_WINDOWS
    const char *p = strrchr(fullpath, '\\');
#else
    const char *p = strrchr(fullpath, '/');
#endif
    CLOG_RET_IF_NULL_X(p, CLOG_INVALID_PARAM, "fullpath is invalid");
    const size_t num = p - fullpath + 1;
    CLOG_RET_IF_X(num > size, CLOG_INVALID_PARAM, "dir buf is too small, size = %zu, num = %zu", size, num);
    dir[num] = '\0';
    return clog_memcpy(dir, size, fullpath, num);
}
