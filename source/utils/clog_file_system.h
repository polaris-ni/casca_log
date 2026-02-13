/**
 * @author Polaris
 * @date 2026/2/12
 */

#ifndef CASCA_LOG_CLOG_FILE_SYSTEM_H
#define CASCA_LOG_CLOG_FILE_SYSTEM_H

#include "casca_log_base.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct clog_file clog_file_t;

typedef enum clog_file_flag {
    CLOG_FILE_NONE = 0, /* no flags */

    CLOG_FILE_WRITE = 1 << 0, /* open for write */
    CLOG_FILE_READ = 1 << 1, /* open for read */

    CLOG_FILE_CREATE = 1 << 2, /* create file always, when CLOG_FILE_EXIST is specified, open file if exists */
    CLOG_FILE_EXIST = 1 << 3, /* open file if exists, open failed if not specified */

    CLOG_FILE_TRUNCATE = 1 << 4, /* truncate file, if not specified, data will be appended to the file */

    CLOG_FILE_SHARED = 1 << 5, /* shared read, write and delete, it maybe not work */

    CLOG_FILE_DIRECT = 1 << 6, /* skip page cache */

    CLOG_FILE_SYNC = 1 << 7, /* sync to file system */
} clog_file_flag_e;

/**
 * open file
 * @param path file path
 * @param flags flag of file, see #clog_file_flag_e
 * @return file handle if open success, NULL if open failed
 */
clog_file_t *clog_file_open(const char *path, uint64_t flags);

/**
 * write data to file
 * @param file file handle
 * @param buf data buffer
 * @param size data size
 * @return CLOG_SUCCESS if write success, other value if write failed
 */
clog_res_e clog_file_write(const clog_file_t *file, const void *buf, size_t size);

/**
 * read data from file
 * @param file file handle
 * @param buf data buffer
 * @param size buffer size
 * @param num read size
 * @return CLOG_SUCCESS if read success, other value if read failed
 */
clog_res_e clog_file_read(const clog_file_t *file, void *buf, size_t size, size_t *num);

/**
 * close file
 * @param file file handle
 */
void clog_file_close(clog_file_t *file);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_FILE_SYSTEM_H */
