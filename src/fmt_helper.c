#include "message.h"
#include <stdio.h>
#include <string.h>

int fmt_RexValue(char *buf, int len, const RexValue_t *v) {
    if (len == 0) {
        return 0;
    }
    switch (v->type) {
    case TSL_DATATYPE_INT:
    case TSL_DATATYPE_ENUM:
        return snprintf(buf, len, "%d", v->u.intVal);
    case TSL_DATATYPE_FLOAT:
        return snprintf(buf, len, "%f", v->u.floatVal);
    case TSL_DATATYPE_DOUBLE:
        return snprintf(buf, len, "%lf", v->u.doubleVal);
    case TSL_DATATYPE_DATE:
        if (len < v->u.strVal.charNum) {
            strncpy(buf, v->u.strVal.str, len);
            return len;
        } else {
            strncpy(buf, v->u.strVal.str, v->u.strVal.charNum);
            return v->u.strVal.charNum;
        }
    case TSL_DATATYPE_BOOL:
        if (v->u.boolVal != 0) {
            return snprintf(buf, len, "true");
        } else {
            return snprintf(buf, len, "false");
        }
    case TSL_DATATYPE_TEXT:
        buf[0] = '"';
        if (len == 1) {
            return 1;
        } else if (len < v->u.strVal.charNum + 2) {
            strncpy(buf + 1, v->u.strVal.str, len - 1);
            return len;
        } else {
            strncpy(buf + 1, v->u.strVal.str, v->u.strVal.charNum);
            buf[v->u.strVal.charNum + 1] = '"';
            return v->u.strVal.charNum + 2;
        }
    case TSL_DATATYPE_STRUCT:
        return snprintf(buf, len, "(struct)");
    case TSL_DATATYPE_ARRAY:
        return snprintf(buf, len, "(array)");
    default:
        return snprintf(buf, len, "(unknown)");
    }
}

int fmt_RexParam(char *buf, int len, const RexParam_t *v) {
    size_t nameLen = strlen(v->name);
    if (len <= nameLen + 3) {
        return 0;
    }
    buf[0] = '"';
    strcpy(buf + 1, v->name);
    buf[nameLen + 1] = '"';
    buf[nameLen + 2] = ':';
    return nameLen + 3 + fmt_RexValue(buf + nameLen + 3, len - nameLen - 3, &v->value);
}

int fmt_RexProperty(char *buf, int len, const RexProperty_t *v) {
    size_t nameLen = strlen(v->name);
    if (len <= nameLen + 3) {
        return 0;
    }
    buf[0] = '"';
    strcpy(buf + 1, v->name);
    buf[nameLen + 1] = '"';
    buf[nameLen + 2] = ':';
    return nameLen + 3 + fmt_RexValue(buf + nameLen + 3, len - nameLen - 3, &v->value);
}

int fmt_RexEvent(char *buf, int len, const RexEvent_t *v) {
    size_t nameLen = strlen(v->name);
    buf[0] = '"';
    strcpy(buf + 1, v->name);
    buf[nameLen + 1] = '"';
    buf[nameLen + 2] = ':';
    buf[nameLen + 3] = '{';
    nameLen += 4;
    for (int i = 0; i < v->paramNum; i++) {
        if (i > 0 && nameLen < len) {
            buf[nameLen] = ',';
            nameLen++;
        }
        nameLen += fmt_RexParam(buf + nameLen, len - nameLen, &(v->paramList[i]));
    }
    if (nameLen < len) {
        buf[nameLen] = '}';
        nameLen++;
    }
    return nameLen;
}

int fmt_RexService(char *buf, int len, const RexService_t *v) {
    size_t nameLen = strlen(v->name);
    buf[0] = '"';
    strcpy(buf + 1, v->name);
    buf[nameLen + 1] = '"';
    buf[nameLen + 2] = ':';
    buf[nameLen + 3] = '{';
    nameLen += 4;
    for (int i = 0; i < v->paramNum; i++) {
        if (i > 0 && nameLen < len) {
            buf[nameLen] = ',';
            nameLen++;
        }
        nameLen += fmt_RexParam(buf + nameLen, len - nameLen, &(v->paramList[i]));
    }
    if (nameLen < len) {
        buf[nameLen] = '}';
        nameLen++;
    }
    return nameLen;
}

int fmt_RexMessage(char *buf, int len, const RexMessage_t *v) {
    int ptr = snprintf(buf, len, "{\"seq\":%d,\"type\":", v->sequence);
    switch (v->type) {
    case MSG_TYPE_SDK_READY:
        ptr += snprintf(buf + ptr, len - ptr, "\"ready\"}");
        break;
    case MSG_TYPE_PAIR_FINISHED:
        ptr += snprintf(buf + ptr, len - ptr, "\"pair_finished\"}");
        break;
    case MSG_TYPE_SCENE_TRIGGERED:
        ptr += snprintf(buf + ptr, len - ptr, "\"scene_triggered\",\"scene_id\":\"%s\",\"status\":%d,\"action\":", v->u.sceneTriggerMsg.sceneId, v->u.sceneTriggerMsg.status);
        switch (v->u.sceneTriggerMsg.action) {
        case SceneAction_TRIGGER:
            ptr += snprintf(buf + ptr, len - ptr, "\"trigger\"}");
            break;
        case SceneAction_ADD:
            ptr += snprintf(buf + ptr, len - ptr, "\"add\"}");
            break;
        case SceneAction_UPDATE:
            ptr += snprintf(buf + ptr, len - ptr, "\"update\"}");
            break;
        case SceneAction_DEL:
            ptr += snprintf(buf + ptr, len - ptr, "\"delete\"}");
            break;
        default:
            ptr += snprintf(buf + ptr, len - ptr, "%d}", v->u.sceneTriggerMsg.action);
            break;
        }
        break;
    case MSG_TYPE_NODE_ADDED:
        ptr += snprintf(buf + ptr, len - ptr, "\"node_added\",\"pk\":\"%s\",\"mac\":\"%s\"}", v->u.includedMsg.productId, v->u.includedMsg.mac);
        break;
    case MSG_TYPE_NODE_REMOVED:
        ptr += snprintf(buf + ptr, len - ptr, "\"node_removed\",\"mac\":\"%s\"}", v->u.excludedMsg.mac);
        break;
    case MSG_TYPE_NODE_CONN_STATE:
        ptr += snprintf(buf + ptr, len - ptr, "\"conn_state\",\"mac\":\"%s\",\"state\":", v->u.connStateMsg.mac);
        switch (v->u.connStateMsg.state) {
        case CONNECT_STATE_ONLINE:
            ptr += snprintf(buf + ptr, len - ptr, "\"online\"}");
            break;
        case CONNECT_STATE_OFFLINE:
            ptr += snprintf(buf + ptr, len - ptr, "\"offline\"}");
            break;
        case CONNECT_STATE_LEAVING:
            ptr += snprintf(buf + ptr, len - ptr, "\"leaving\"}");
            break;
        case CONNECT_STATE_DELETING:
            ptr += snprintf(buf + ptr, len - ptr, "\"deleting\"}");
            break;
        default:
            ptr += snprintf(buf + ptr, len - ptr, "%d}", v->u.connStateMsg.state);
            break;
        }
        break;
    case MSG_TYPE_NODE_HEARTBEAT:
        ptr += snprintf(buf + ptr, len - ptr, "\"heartbeat\",\"mac\":\"%s\"}", v->u.heartbeatMsg.mac);
        break;
    case MSG_TYPE_NODE_EVENT:
        ptr += snprintf(buf + ptr, len - ptr, "\"event\",\"pk\":\"%s\",\"mac\":\"%s\",\"event\":{", v->u.eventMsg.productId, v->u.eventMsg.mac);
        ptr += fmt_RexEvent(buf + ptr, len - ptr, v->u.eventMsg.event);
        if (ptr < len) {
            buf[ptr++] = '}';
            if (ptr < len) {
                buf[ptr++] = '}';
            }
        }
        break;
    case MSG_TYPE_NODE_STATE_CHANGED:
        ptr += snprintf(buf + ptr, len - ptr, "\"property\",\"pk\":\"%s\",\"mac\":\"%s\",\"properties\":{", v->u.propertyMsg.productId, v->u.propertyMsg.mac);
        for (int i = 0; i < v->u.propertyMsg.propertyNum; i++) {
            if (i > 0 && ptr < len) {
                buf[ptr++] = ',';
            }
            ptr += fmt_RexProperty(buf + ptr, len - ptr, &v->u.propertyMsg.propertyList[i]);
        }
        if (ptr < len) {
            buf[ptr++] = '}';
            if (ptr < len) {
                buf[ptr++] = '}';
            }
        }
        break;
    case MSG_TYPE_OTA_EVENT:
        ptr += snprintf(buf + ptr, len - ptr, "\"ota_event\",\"step\":%d}", v->u.otaProgressMsg.step);
        break;
    case MSG_TYPE_SERVICE:
        ptr += snprintf(buf + ptr, len - ptr, "\"service\",\"pk\":\"%s\",\"mac\":\"%s\",\"service\":{", v->u.serviceMsg.productId, v->u.serviceMsg.mac);
        ptr += fmt_RexService(buf + ptr, len - ptr, v->u.serviceMsg.service);
        if (ptr < len) {
            buf[ptr++] = '}';
            if (ptr < len) {
                buf[ptr++] = '}';
            }
        }
        break;
    case MSG_TYPE_EXECUTE_RESULT:
        ptr += snprintf(buf + ptr, len - ptr, "\"execute_result\",\"result\":");
        switch (v->u.resultMsg.result) {
        case SUCCESS:
            ptr += snprintf(buf + ptr, len - ptr, "\"success\"}");
            break;
        case FAILURE:
            ptr += snprintf(buf + ptr, len - ptr, "\"failure\"}");
            break;
        case INVALID_ARG:
            ptr += snprintf(buf + ptr, len - ptr, "\"invalid_arg\"}");
            break;
        case ALREADY_JOINED:
            ptr += snprintf(buf + ptr, len - ptr, "\"already_joined\"}");
            break;
        case OVERTIME:
            ptr += snprintf(buf + ptr, len - ptr, "\"timeout\"}");
            break;
        case MEMORY_ALLOC_ERROR:
            ptr += snprintf(buf + ptr, len - ptr, "\"mem_error\"}");
            break;
        default:
            ptr += snprintf(buf + ptr, len - ptr, "%d}", v->u.resultMsg.result);
            break;
        }
        break;
    case MSG_TYPE_USER_DEFINED:
        ptr += snprintf(buf + ptr, len - ptr, "\"user_defined\",\"message\":\"%.*s\"}", v->u.userDefinedMsg.length, v->u.userDefinedMsg.message);
        break;
    default:
        ptr += snprintf(buf + ptr, len - ptr, "%d}", v->type);
        break;
    }
    return ptr;
}