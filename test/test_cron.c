#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define elog_e(tag, ...) printf(__VA_ARGS__)

static void rex_scene_mgr_split(char *src, const char *separator, char **dest, int *num) {
    int   count = 0;
    char *pNext = NULL;
    if (src == NULL || strlen(src) == 0) // 如果传入的地址为空或长度为0，直接终止
    {
        elog_e(LOG_TAG_SCENE_MGR, "split,src string is null!");
        return;
    }
    if (separator == NULL || strlen(separator) == 0) // 如未指定分割的字符串，直接终止
    {
        elog_e(LOG_TAG_SCENE_MGR, "split,separator string is null!");
        return;
    }
    pNext = (char *)strtok(src, separator); // 注意:strtok会更改源字符串
    while (pNext != NULL) {
        *dest++ = pNext;
        ++count;
        pNext = (char *)strtok(NULL, separator);
    }
    *num = count;
}

static int rex_scene_mgr_get_localtime(int *timeArray, int arrayCount) {
    time_t     now = time(NULL);
    struct tm *p   = localtime(&now);
    if (arrayCount > 7) {
        timeArray[0] = p->tm_sec;
        timeArray[1] = p->tm_min;
        timeArray[2] = p->tm_hour;
        timeArray[3] = p->tm_mday;
        timeArray[4] = p->tm_mon + 1;
        timeArray[5] = p->tm_wday;
        timeArray[6] = p->tm_year + 1900;
    } else {
        return -1;
    }
    return 0;
}

static int rex_scene_mgr_week_translation(char *week) {
    int w = -1;
    if (0 == strcasecmp(week, "MON")) {
        w = 1;
    } else if (0 == strcasecmp(week, "TUE")) {
        w = 2;
    } else if (0 == strcasecmp(week, "WED")) {
        w = 3;
    } else if (0 == strcasecmp(week, "THU")) {
        w = 4;
    } else if (0 == strcasecmp(week, "FRI")) {
        w = 5;
    } else if (0 == strcasecmp(week, "SAT")) {
        w = 6;
    } else if (0 == strcasecmp(week, "SUN")) {
        w = 0;
    } else if (0 == strcasecmp(week, "*")) {
        w = 7;
    } else if (0 == strcasecmp(week, "?")) {
        w = 7;
    }
    return w;
}

static int rex_scene_mgr_proc_cron(char *cron) {
    int   cronNum          = 0;
    int   cronTimer        = 0;
    char *cronArrayTime[8] = {0}; //{分钟} {小时} {日期} {月份} {星期} {年份(可为空)}
    int   curTime[8]       = {0};

    rex_scene_mgr_split(cron, " ", cronArrayTime, &cronNum);
    if (0 != rex_scene_mgr_get_localtime(curTime, sizeof(curTime))) // 秒/分/时/日/月/星期/年
    {
        elog_e(LOG_TAG_SCENE_MGR, "scene:get localtime failed! cron=%s", cron);
        return -1;
    }
    if (cronNum != 5 && cronNum != 6) {
        elog_e(LOG_TAG_SCENE_MGR, "scene timer cron parse failed!not support cron=%s", cron);
        return -1;
    }

    for (int idx = cronNum - 1; idx >= 0; --idx) {
        cronTimer = -1;
        if (idx == 4) // 星期，单独处理
        {
            int   found = 0;
            int   w_num = 0;
            char *w[7]  = {0};

            rex_scene_mgr_split(cronArrayTime[4], ",", w, &w_num);
            for (int i = 0; i < w_num; ++i) {
                cronTimer = rex_scene_mgr_week_translation(w[i]);
                if (curTime[idx + 1] == cronTimer || cronTimer == 7) {
                    found = 1;
                    break;
                }
            }
            if (found == 0) {
                elog_e(LOG_TAG_SCENE_MGR, "scene:week=[%s] condition not satisfied!", cronArrayTime[4]);
                return -1;
            }
        } else {
            if ((strcmp(cronArrayTime[idx], "*")) && (strcmp(cronArrayTime[idx], "?")) && (sscanf(cronArrayTime[idx], "%d", &cronTimer) != 1)) {
                elog_e(LOG_TAG_SCENE_MGR, "scene timer cron parse failed!not support cron=%s,idx=%d", cronArrayTime[idx], idx);
                return -1;
            }
            if (cronTimer != -1 && cronTimer != curTime[idx + 1]) {
                // elog_d(LOG_TAG_SCENE_MGR, "cron time=%d is not equal system time's time=%d", cronTimer, curTime[idx + 1]);
                return -1;
            }
        }
    }
    return 0;
}

typedef double        eyecatcherType;
static eyecatcherType eyecatcher = (eyecatcherType)0x8888888888888888;
enum LOG_LEVELS {
    INVALID_LEVEL = -1,
    TRACE_MAXIMUM = 1,
    TRACE_MEDIUM,
    TRACE_MINIMUM,
    TRACE_PROTOCOL,
    LOG_ERROR,
    LOG_SEVERE,
    LOG_FATAL,
};
static char        msg_buf[512];
static const char *protocol_message_list[] = {
    "%d %s -> CONNECT version %d clean: %d (%d)",                                           /* 0, was 131, 68 and 69 */
    "%d %s <- CONNACK rc: %d",                                                              /* 1, was 132 */
    "%d %s -> CONNACK rc: %d (%d)",                                                         /* 2, was 138 */
    "%d %s <- PINGREQ",                                                                     /* 3, was 35 */
    "%d %s -> PINGRESP (%d)",                                                               /* 4 */
    "%d %s <- DISCONNECT",                                                                  /* 5 */
    "%d %s <- SUBSCRIBE msgid: %d",                                                         /* 6, was 39 */
    "%d %s -> SUBACK msgid: %d (%d)",                                                       /* 7, was 40 */
    "%d %s <- UNSUBSCRIBE msgid: %d",                                                       /* 8, was 41 */
    "%d %s -> UNSUBACK msgid: %d (%d)",                                                     /* 9 */
    "%d %s -> PUBLISH msgid: %d qos: %d retained: %d rc %d payload len(%d): %.*s",          /* 10, was 42 */
    "%d %s <- PUBLISH msgid: %d qos: %d retained: %d payload len(%d): %.*s",                /* 11, was 46 */
    "%d %s -> PUBACK msgid: %d (%d)",                                                       /* 12, was 47 */
    "%d %s -> PUBREC msgid: %d (%d)",                                                       /* 13, was 48 */
    "%d %s <- PUBACK msgid: %d",                                                            /* 14, was 49 */
    "%d %s <- PUBREC msgid: %d",                                                            /* 15, was 53 */
    "%d %s -> PUBREL msgid: %d (%d)",                                                       /* 16, was 57 */
    "%d %s <- PUBREL msgid %d",                                                             /* 17, was 58 */
    "%d %s -> PUBCOMP msgid %d (%d)",                                                       /* 18, was 62 */
    "%d %s <- PUBCOMP msgid:%d",                                                            /* 19, was 63 */
    "%d %s -> PINGREQ (%d)",                                                                /* 20, was 137 */
    "%d %s <- PINGRESP",                                                                    /* 21, was 70 */
    "%d %s -> SUBSCRIBE msgid: %d (%d)",                                                    /* 22, was 72 */
    "%d %s <- SUBACK msgid: %d",                                                            /* 23, was 73 */
    "%d %s <- UNSUBACK msgid: %d",                                                          /* 24, was 74 */
    "%d %s -> UNSUBSCRIBE msgid: %d (%d)",                                                  /* 25, was 106 */
    "%d %s <- CONNECT",                                                                     /* 26 */
    "%d %s -> PUBLISH qos: 0 retained: %d rc: %d payload len(%d): %.*s",                    /* 27 */
    "%d %s -> DISCONNECT (%d)",                                                             /* 28 */
    "Socket error for client identifier %s, socket %d, peer address %s; ending connection", /* 29 */
    "%d %s <- DISCONNECT (%d)",                                                             /* 30 */
};
static const char *trace_message_list[] = {
    "Failed to remove client from bstate->clients",                                                                /* 0 */
    "Removed client %s from bstate->clients, socket %d",                                                           /* 1 */
    "Packet_Factory: unhandled packet type %d",                                                                    /* 2 */
    "Packet %s received from client %s for message identifier %d, but no record of that message identifier found", /* 3 */
    "Packet %s received from client %s for message identifier %d, but message is wrong QoS, %d",                   /* 4 */
    "Packet %s received from client %s for message identifier %d, but message is in wrong state",                  /* 5 */
    "%s received from client %s for message id %d - removing publication",                                         /* 6 */
    "Trying %s again for client %s, socket %d, message identifier %d",                                             /* 7 */
    "",                                                                                                            /* 8 */
    "(%lu) %*s(%d)> %s:%d",                                                                                        /* 9 */
    "(%lu) %*s(%d)< %s:%d",                                                                                        /* 10 */
    "(%lu) %*s(%d)< %s:%d (%d)",                                                                                   /* 11 */
    "Storing unsent QoS 0 message",                                                                                /* 12 */
};
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

const char *Messages_get(int index, enum LOG_LEVELS log_level) {
    const char *msg = NULL;

    if (log_level == TRACE_PROTOCOL)
        msg = (index >= 0 && index < ARRAY_SIZE(protocol_message_list)) ? protocol_message_list[index] : NULL;
    else
        msg = (index >= 0 && index < ARRAY_SIZE(trace_message_list)) ? trace_message_list[index] : NULL;
    return msg;
}

void Log(enum LOG_LEVELS log_level, int msgno, const char *format, ...) {
    // if (log_level >= trace_settings.trace_level)
    {
        const char *temp = NULL;
        va_list     args;

        /* we're using a static character buffer, so we need to make sure only one thread uses it at a time */
        // Thread_lock_mutex(log_mutex);
        if (format == NULL && (temp = Messages_get(msgno, log_level)) != NULL)
            format = temp;

        va_start(args, format);
        vsnprintf(msg_buf, sizeof(msg_buf), format, args);

        // Log_trace(log_level, msg_buf);
        printf("%s", msg_buf);
        va_end(args);
        // Thread_unlock_mutex(log_mutex);
    }
}

static void checkEyecatchers(char *file, int line, void *p, size_t size) {
    eyecatcherType    *sp = (eyecatcherType *)p;
    char              *cp = (char *)p;
    eyecatcherType     us;
    static const char *msg = "Invalid %s eyecatcher %lf in heap item at file %s line %d";

    if ((us = *--sp) != eyecatcher)
        Log(LOG_ERROR, 13, msg, "start", us, file, line);

    cp += size;
    if ((us = *(eyecatcherType *)cp) != eyecatcher)
        Log(LOG_ERROR, 13, msg, "end", us, file, line);
}

int main(int argc, char **argv) {
    if (argc > 1) {
        rex_scene_mgr_proc_cron(argv[1]);
    } else {
        char buf[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0xE1, 0x43, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00};
        checkEyecatchers(__FILE__, __LINE__, &buf[8], 0);
    }
}
