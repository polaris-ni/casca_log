/**
 * @author Polaris
 * @date 2026/2/12
 */

#include "clog_file_system.h"
#include "clog_error.h"
#include "clog_hooks.h"
#include "clog_platform.h"
#ifdef CLOG_PLATFORM_WINDOWS
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/file.h>
#include <unistd.h>
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
static wchar_t *clog_utf8_to_utf16(const char *utf8_str) {
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

clog_file_t *clog_file_open(const char *path, uint64_t flags) {
    CLOG_RET_IF_NULL_X(path, NULL, "path is NULL");
    CLOG_RET_IF_X(flags == CLOG_FILE_NONE, NULL, "flags is CLOG_FILE_NONE");
#ifdef CLOG_PLATFORM_WINDOWS
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
                  (uint64_t) GetLastError());

    if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_WRITE) && !CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_TRUNCATE)) {
        if (SetFilePointer(handle, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER) {
            CLOG_ERR_ADD("SetFilePointer failed, err = 0x%llX", (uint64_t) GetLastError());
            CloseHandle(handle);
            return NULL;
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
        flag |= O_CREAT | O_EXCL;
        if (!CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_EXIST)) {
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
    mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR;
    if (CLOG_FILE_CHECK_FLAG(flags, CLOG_FILE_SHARED)) {
        mode |= S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    }
    const int fd = open(path, flag, mode);
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

clog_res_e clog_file_write(const clog_file_t *file, const void *buf, size_t size) {
    CLOG_RET_IF_NULL_X(file, CLOG_INVALID_PARAM, "file is NULL");
    CLOG_RET_IF_NULL_X(buf, CLOG_INVALID_PARAM, "buf is NULL");
    CLOG_RET_IF_X(size == 0, CLOG_INVALID_PARAM, "buf size is 0");
    CLOG_RET_IF_X(!CLOG_FILE_CHECK_FLAG(file->flags, CLOG_FILE_WRITE), CLOG_NOT_SUPPORTED, "file not opened for write");
#ifdef CLOG_PLATFORM_WINDOWS
    CLOG_RET_IF_X(file->handle == INVALID_HANDLE_VALUE, CLOG_INVALID_PARAM, "file handle is invalid");
    if (!WriteFile(file->handle, buf, size, NULL, NULL)) {
        CLOG_ERR_ADD("WriteFile failed, err = 0x%llX", (uint64_t) GetLastError());
        return CLOG_FAIL;
    }
    if (CLOG_FILE_CHECK_FLAG(file->flags, CLOG_FILE_SYNC)) {
        if (!FlushFileBuffers(file->handle)) {
            CLOG_ERR_ADD("WARN: FlushFileBuffers failed, size = %zu, err = 0x%llX", size, (uint64_t) GetLastError());
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

clog_res_e clog_file_read(const clog_file_t *file, void *buf, size_t size, size_t *num) {
    CLOG_RET_IF_NULL_X(file, CLOG_INVALID_PARAM, "file is NULL");
    CLOG_RET_IF_NULL_X(buf, CLOG_INVALID_PARAM, "buf is NULL");
    CLOG_RET_IF_X(size == 0, CLOG_INVALID_PARAM, "buf size is 0");
    CLOG_RET_IF_X(!CLOG_FILE_CHECK_FLAG(file->flags, CLOG_FILE_READ), CLOG_NOT_SUPPORTED, "file not opened for read");
#ifdef CLOG_PLATFORM_WINDOWS
    CLOG_RET_IF_X(file->handle == INVALID_HANDLE_VALUE, CLOG_INVALID_PARAM, "file handle is invalid");
    DWORD read_size = 0;
    if (!ReadFile(file->handle, buf, size, &read_size, NULL)) {
        CLOG_ERR_ADD("ReadFile failed, size = %zu, err = 0x%llX", size, (uint64_t) GetLastError());
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

void clog_file_close(clog_file_t *file) {
    CLOG_RET_VOID_IF_NULL(file);
#ifdef CLOG_PLATFORM_WINDOWS
    CloseHandle(file->handle);
#else
    CLOG_IGNORE_RES(close(file->fd));
#endif
    clog_free(file);
}
