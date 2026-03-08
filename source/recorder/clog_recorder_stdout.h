/**
 * @author polaris
 * @date  2025/10/16
 */
#ifndef CASCA_LOG_CLOG_RECORDER_STDOUT_H
#define CASCA_LOG_CLOG_RECORDER_STDOUT_H

#include "clog_recorder.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define CLOG_COLOR_OF(r, g, b) ((uint32_t)((((r) & 0xFF) << 16) | (((g) & 0xFF) << 8) | (((b) & 0xFF))))
#define CLOG_COLOR_GET_R(value) ((uint8_t)((((value) & 0xFFFFFF) >> 16) & 0xFF))
#define CLOG_COLOR_GET_G(value) ((uint8_t)((((value) & 0xFFFF) >> 8) & 0xFF))
#define CLOG_COLOR_GET_B(value) ((uint8_t)((value) & 0xFF))
#define CLOG_COLOR_UNSPECIFIED 0xFF000000U

typedef enum clog_stdout_color_mode {
    CLOG_RECORDER_STDOUT_COLOR_OFF = 0, /* disabled color */
    CLOG_RECORDER_STDOUT_COLOR_BASIC = 1, /* basic ansi colors */
    CLOG_RECORDER_STDOUT_COLOR_BASIC_ENHANCED = 2, /* basic ansi colors and high intensity colors */
    CLOG_RECORDER_STDOUT_COLOR_256 = 3, /* 256-color mode */
    CLOG_RECORDER_STDOUT_COLOR_TRUE = 4, /* 24-bit - RGB */
    CLOG_RECORDER_STDOUT_COLOR_MAX
} clog_stdout_color_mode_e;

/**
 *   colors values:
 *       # mode 1 & mode 2: [color_value], just one value to set basic color， the value of colors are as follows:
 *               color   foreground_value    background_value        support mode
 *               black           30              40                  mode 1 & 2
 *               red             31              41                  mode 1 & 2
 *               green           32              42                  mode 1 & 2
 *               yellow          33              43                  mode 1 & 2
 *               blue            34              44                  mode 1 & 2
 *               magenta         35              45                  mode 1 & 2
 *               cyan            36              46                  mode 1 & 2
 *               white           37              47                  mode 1 & 2
 *               bright black    90              100                 mode 2 only
 *               bright red      91              101                 mode 2 only
 *               bright green    92              102                 mode 2 only
 *               bright yellow   93              103                 mode 2 only
 *               bright blue     94              104                 mode 2 only
 *               bright magenta  95              105                 mode 2 only
 *               bright cyan     96              106                 mode 2 only
 *               bright white    97              107                 mode 2 only
 *       # mode 3: [color_value], use precise 256 color values, please refer to Wikipedia
 *       # mode 4: [r, g, b], use three values(R, G, B primary colors) to combine the final color value
 */
typedef struct clog_recorder_stdout_color_value {
    uint32_t foreground_color;
    uint32_t background_color;
} clog_recorder_stdout_color_value_t;

typedef struct clog_recorder_stdout_attr {
    clog_stdout_color_mode_e mode;
    clog_recorder_stdout_color_value_t colors[CLOG_LEVEL_NUM];
} clog_recorder_stdout_attr_t;

/**
 * create stdout recorder
 * @param attr stdout recorder attr
 * @return #clog_recorder_t
 */
clog_recorder_t *clog_recorder_stdout_create(const clog_recorder_stdout_attr_t *attr);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_RECORDER_STDOUT_H */
