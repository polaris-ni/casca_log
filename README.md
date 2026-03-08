# Casca Log
Casca Log是一个纯C语言的日志框架，希望成为一个简单易用、扩展性好、功能丰富、性能优秀的跨平台的日志库。

Casca Log is a logging framework implemented purely in C, designed to be a simple, easy-to-use, highly extensible, feature-rich, high-performance, and cross-platform logging library.
## Framework Overview / 框架简介
Casca Log的框架结构如下，划分为过滤器（Filter）、格式化器（Formatter）、日志通道（Channel）、分发器（Dispatcher）、记录器（Recorder）等模块。调用者（Caller）在打印日志时，首先会生成`clog_item_t`，包含日志的模块、目的地、级别、时间、内容等信息，然后调用`前向过滤器`进行过滤。经过`前向过滤器`后，会使用`格式化器`对日志进行格式化。在经过`后置过滤器`处理后，日志会被加入`通道`中，并向`分发器`发送日志到达事件。`分发器`从通道中读取日志并调用`记录器`进行记录。

The framework structure of `Casca Log` is organized into several core modules: Filter, Formatter, Channel, Dispatcher, and Recorder. When printing a log, the Caller first generates a clog_item_t structure containing metadata such as the log’s module, destination, level, timestamp, and content. This log item is then passed to the prefilter for preliminary filtering. After filtering, the log is formatted by the formatter, after processed by postfilter, it will be enqueued into the channel, and a log arrival event is dispatched to the dispatcher. Finally, the dispatcher retrieves the log from the channel and invokes the recorder to persist it.
![CascaLogFramework.svg](docs/images/CascaLogFramework.svg)

## How to use / 如何使用
```c
    clog_context_t *context;
    clog_res_e ret = clog_context_create(process, &context);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_set_mask(context, CLOG_LEVEL_ALL);
    ret = clog_set_buffer_pool(context, true, 128, 75); // using buffer pool, optionally
    ASSERT_EQ(ret, CLOG_SUCCESS); // if failed, you should call clog_context_destroy
    clog_module_t module = {"test", CLOG_LEVEL_ALL, 2, {CLOG_RECORDER_ID_STDOUT, CLOG_RECORDER_ID_FILE}};
    ret = clog_add_module(context, &module); // add module of process
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_add_filter(context, clog_filter_keywords_create(1, "password")); // add log filter, optionally
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_set_log_format(context,
                              "{_year}-{_month}-{_day} {_hour}:{_minute}:{_second}.{_millisecond} {_level} "
                              "[{_process}.{_module}:{_tid} {_file}:{_line}] {_content}{_ln}"); // set log output format
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_channel_t *channel = clog_atomic_queue_channel_create(); // set channel
    ASSERT_NE(channel, nullptr);
    ret = clog_set_channel(context, channel);
    ASSERT_EQ(ret, CLOG_SUCCESS); // if failed, you should call clog_channel_destroy(&channel)
    clog_dispatcher_t *dispatcher = clog_dispatcher_async_thread_create();
    ret = clog_set_dispatcher(context, dispatcher);
    ASSERT_EQ(ret, CLOG_SUCCESS); // if failed, you chould call clog_dispatcher_destroy(&dispatcher)
    clog_recorder_stdout_attr_t stdout_attr = {
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
    ret = clog_add_recorder(context, clog_recorder_stdout_create(&stdout_attr)); // add stdout recorder, log will be output to stdout
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_recorder_file_attr_t file_attr = {
        "{_cwd}{_path_separator}logs{_path_separator}metainfo",
        "{_cwd}{_path_separator}logs{_path_separator}{_year}{_month}{_day}{_path_separator}",
        "log_{_log_index}_{_year}{_month}{_day}_{_hour}{_minute}{_second}.log",
        {CLOG_RECORDER_FILE_SPLIT_SIZE, 1048576},
    };
    ret = clog_add_recorder(context, clog_recorder_file_create(&file_attr)); // add file recorder, log will be output to file
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_context_setup(context); // final setup
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_TRACE, "test trace log"); // now you can use log
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_DEBUG, "test debug log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_INFO, "test info log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_WARN, "test warn log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_ERROR, "test error log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_FETAL, "test fetal log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_INFO, "filter password");
    ASSERT_EQ(ret, CLOG_NOT_PERMITTED);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_INFO, "hello, %s!", "world");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_context_destroy(&context);
```