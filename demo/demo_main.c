/**
 * @author Polaris
 * @date 2026/3/16
 */
#include "casca_log_core.h"
#include "clog_channel_atomic_queue.h"
#include "clog_dispatcher_async_thread.h"
#include "clog_filter_keywords.h"
#include "clog_recorder_stdout.h"

/* optionally, define customized log macros */
#define M_LOG_T(context, module, fmt, ...) CLOG_MODULE_LOG(context, module, CLOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#define M_LOG_D(context, module, fmt, ...) CLOG_MODULE_LOG(context, module, CLOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define M_LOG_I(context, module, fmt, ...) CLOG_MODULE_LOG(context, module, CLOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define M_LOG_W(context, module, fmt, ...) CLOG_MODULE_LOG(context, module, CLOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define M_LOG_E(context, module, fmt, ...) CLOG_MODULE_LOG(context, module, CLOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define M_LOG_F(context, module, fmt, ...) CLOG_MODULE_LOG(context, module, CLOG_LEVEL_FETAL, fmt, ##__VA_ARGS__)

static clog_res_e casca_log_init(clog_context_t **context)
{
    clog_context_t *tmp = NULL;
    clog_res_e ret = clog_context_create("casca_log_demo", &tmp);
    CLOG_CLEAN_RET_IF_FAILED(ret, clog_context_destroy(&tmp));
    clog_set_mask(tmp, CLOG_LEVEL_ALL);
    /* optionally, use buffer pool to improve performance */
    ret = clog_set_buffer_pool(tmp, true, 128, 75);
    CLOG_CLEAN_RET_IF_FAILED(ret, clog_context_destroy(&tmp));
    /* add module of process */
    const clog_module_t module = {"test", CLOG_LEVEL_ALL, 2, {CLOG_RECORDER_ID_STDOUT, CLOG_RECORDER_ID_FILE}};
    ret = clog_add_module(tmp, &module);
    CLOG_CLEAN_RET_IF_FAILED(ret, clog_context_destroy(&tmp));
    /* optionally, add "password" filter */
    clog_filter_t *filter = clog_filter_keywords_create(1, "password");
    ret = clog_add_filter(tmp, filter);
    CLOG_CLEAN_RET_IF_FAILED(ret, (clog_filter_free(filter), clog_context_destroy(&tmp)));
    /* set log output format */
    ret = clog_set_log_format(tmp,
                              "{_year}-{_month}-{_day} {_hour}:{_minute}:{_second}.{_millisecond} {_level} "
                              "[{_process}.{_module}:{_tid} {_file}:{_line}] {_content}{_ln}");
    CLOG_CLEAN_RET_IF_FAILED(ret, clog_context_destroy(&tmp));
    /* set channel */
    clog_channel_t *channel = clog_atomic_queue_channel_create();
    ret = clog_set_channel(tmp, channel);
    CLOG_CLEAN_RET_IF_FAILED(ret, (clog_channel_destroy(&channel), clog_context_destroy(&tmp)));
    /* set dispatcher */
    clog_dispatcher_t *dispatcher = clog_dispatcher_async_thread_create();
    ret = clog_set_dispatcher(tmp, dispatcher);
    CLOG_CLEAN_RET_IF_FAILED(ret, (clog_dispatcher_destroy(&dispatcher), clog_context_destroy(&tmp)));
    /* add stdout recorder, log will be output to stdout */
    const clog_recorder_stdout_attr_t stdout_attr = {
        CLOG_RECORDER_STDOUT_COLOR_TRUE,
        {
            {CLOG_COLOR_OF(187, 187, 187), CLOG_COLOR_UNSPECIFIED},
            {CLOG_COLOR_OF(0, 112, 187), CLOG_COLOR_UNSPECIFIED},
            {CLOG_COLOR_OF(72, 187, 49), CLOG_COLOR_UNSPECIFIED},
            {CLOG_COLOR_OF(243, 156, 17), CLOG_COLOR_UNSPECIFIED},
            {CLOG_COLOR_OF(255, 0, 0), CLOG_COLOR_UNSPECIFIED},
            {CLOG_COLOR_OF(255, 255, 255), CLOG_COLOR_OF(139, 0, 0)},
        },
    };
    clog_recorder_t *recorder = clog_recorder_stdout_create(&stdout_attr);
    ret = clog_add_recorder(tmp, recorder);
    CLOG_CLEAN_RET_IF_FAILED(ret, (clog_recorder_destroy(&recorder), clog_context_destroy(&tmp)));
    /* finish all setups */
    ret = clog_context_setup(tmp);
    CLOG_CLEAN_RET_IF_FAILED(ret, clog_context_destroy(&tmp));
    *context = tmp;
    return CLOG_SUCCESS;
}

int main(void)
{
    clog_context_t *context = NULL;
    const clog_res_e ret = casca_log_init(&context);
    CLOG_RET_IF_FAILED(ret);

    const char *log_messages[] = {
        "This is a trace log message.",   "This is a debug log message.",  "This is an info log message.",
        "This is a warning log message.", "This is an error log message.", "This is a fetal log message.",
    };

    M_LOG_T(context, "test", log_messages[0]);
    M_LOG_D(context, "test", log_messages[1]);
    M_LOG_I(context, "test", log_messages[2]);
    M_LOG_W(context, "test", log_messages[3]);
    M_LOG_E(context, "test", log_messages[4]);
    M_LOG_F(context, "test", log_messages[5]);

    clog_context_destroy(&context);
    return 0;
}