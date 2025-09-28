/**
 * @author Polaris
 * @date  2025/9/3
 */
#ifndef CASCA_LOG_CASCA_LOG_CONFIGS_H
#define CASCA_LOG_CASCA_LOG_CONFIGS_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define CASCA_LOG_DEBUG 1 /* 0 means debug mode disabled, other values mean enabled */
#define CASCA_LOG_HOOKS 1 /* enable hook in casca log */
#define CASCA_LOG_MEM_POOL 1 /* enable mem pool to improve memory allocator performance */
#ifndef CASCA_LOG_ERR_BUF_SIZE
#define CASCA_LOG_ERR_BUF_SIZE 256
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_CONFIGS_H */
