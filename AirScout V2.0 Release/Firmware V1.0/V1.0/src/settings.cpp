#include "settings.h"

#include "globals.h"
#include "ulog.h"

extern "C" {
#include <cJSON.h>
}

#include <ff.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/fs.h"

Settings::Settings(const char *path) {
    m_path = (char *)malloc(strlen(path) + 1);

    m_data = cJSON_CreateObject();
    strcpy(m_path, path);
}

Settings::~Settings() {
    cJSON_Delete(m_data);
    free(m_path);
}

SETSTATUS Settings::loadSettings() {
    if(m_read(m_path) != SETSTATUS::OK) {
        ulog_warn("Settings file not found. Loading Defaults...");
        if(m_loadDefaults() != SETSTATUS::OK) {
            ulog_error("No Settings Files found");
            m_data = NULL;
            return SETSTATUS::FILE_NOT_FOUND;
        }
        saveToDisk();
    }
    ulog_info("Settings Loaded");
    // char *str = cJSON_Print(m_data);
    // ulog_trace("%s", str);
    // free(str);
    return SETSTATUS::OK;
}

SETSTATUS Settings::m_loadDefaults() {
    m_data = cJSON_Parse(DEFAULT_SETTINS_STR);
    return SETSTATUS::OK;
}

SETSTATUS Settings::m_getobjat(const char *key, cJSON *obj) {
    char *tmp = (char *)malloc(strlen(key) + 1);

    strcpy(tmp, key);
    char *token = strtok(tmp, "/");

    if(token == NULL) {
        return SETSTATUS::DOES_NOT_EXIST;
    }

    *obj = *cJSON_GetObjectItemCaseSensitive(m_data, token);
    while(true) {
        if(cJSON_IsNull(obj)) {
            free(tmp);
            return SETSTATUS::DOES_NOT_EXIST;
        }
        token = strtok(NULL, "/");
        if(token == NULL) {  // reached the last token -> this is the value we want
            free(tmp);
            return SETSTATUS::OK;
        }
        *obj = *cJSON_GetObjectItemCaseSensitive(obj, token);  // go recursively down the data tree
    }
}

SETSTATUS Settings::getint(const char *key, int32_t *value) {
    if(!m_check_loaded()) {
        return SETSTATUS::FILE_ERROR;
    }
    cJSON obj;
    SETSTATUS ret = m_getobjat(key, &obj);
    if(ret != SETSTATUS::OK) {
        return SETSTATUS::DOES_NOT_EXIST;
    }

    if(!cJSON_IsNumber(&obj)) {
        return SETSTATUS::WRONG_TYPE;
    }
    *value = obj.valueint;
    return SETSTATUS::OK;
}

// must free value. strptr should be allocated or NULL before
SETSTATUS Settings::getString(const char *key, char **strptr) {
    if(!m_check_loaded()) {
        return SETSTATUS::FILE_ERROR;
    }
    cJSON obj;
    SETSTATUS ret = m_getobjat(key, &obj);
    if(ret != SETSTATUS::OK) {
        return SETSTATUS::DOES_NOT_EXIST;
    }

    if(!cJSON_IsString(&obj) || cJSON_IsNull(&obj)) {
        return SETSTATUS::WRONG_TYPE;
    }
    *strptr = (char *)malloc(strlen(obj.valuestring) + 1);
    if(*strptr == NULL) {
        return SETSTATUS::ALLOCATION_ERROR;
    }
    strcpy(*strptr, obj.valuestring);
    return SETSTATUS::OK;
}

SETSTATUS Settings::getBool(const char *key, bool *value) {
    if(!m_check_loaded()) {
        return SETSTATUS::FILE_ERROR;
    }
    cJSON obj;
    SETSTATUS ret = m_getobjat(key, &obj);
    if(ret != SETSTATUS::OK) {
        return SETSTATUS::DOES_NOT_EXIST;
    }

    if(!cJSON_IsBool(&obj)) {
        return SETSTATUS::WRONG_TYPE;
    }
    *value = obj.valueint;
    return SETSTATUS::OK;
}

SETSTATUS Settings::getDouble(const char *key, double *value) {
    if(!m_check_loaded()) {
        return SETSTATUS::FILE_ERROR;
    }
    cJSON obj;
    SETSTATUS ret = m_getobjat(key, &obj);
    if(ret != SETSTATUS::OK) {
        return SETSTATUS::DOES_NOT_EXIST;
    }
    if(!cJSON_IsNumber(&obj)) {
        return SETSTATUS::WRONG_TYPE;
    }
    *value = obj.valuedouble;
    return SETSTATUS::OK;
}

cJSON *Settings::m_createPathAndGetLast(const char *key) {
    if(!strchr(key, '/')) {
        // '/' not found
        
        
        if(cJSON_HasObjectItem(m_data, key)) {
            cJSON_DeleteItemFromObject(m_data, key);  // we don't want duplicates, so it is removed
            printf("Deleting item %s", key);
            return m_data;                              // return the last object
        } else {
            return m_data;                              // return the last object
        }
    }

    char *tmp = (char *)malloc(strlen(key) + 1);
    char *token = NULL;
    char *nextToken = NULL;
    printf("Key: %s\n", key);
    memcpy(tmp, key, strlen(key) + 1);
    nextToken = strtok(tmp, "/");

    cJSON *obj;
    printf("Checking item: %s...", nextToken);
    if(cJSON_HasObjectItem(m_data, nextToken)) {
        printf("Item found\n");
        obj = cJSON_GetObjectItemCaseSensitive(m_data, nextToken);
    } else {
        printf("Item not found\n");
        obj = cJSON_AddObjectToObject(m_data, nextToken);
    }

    nextToken = strtok(NULL, "/");
    while(true) {
        token = nextToken;
        nextToken = strtok(NULL, "/");

        if(nextToken == NULL) {
            // last object, this is the end
            if(cJSON_HasObjectItem(obj, token)) {
                cJSON_DeleteItemFromObject(obj, token);  // we don't want duplicates, so it is removed
                printf("Deleting item %s", token);
                free(tmp);
                return obj;                              // return the last object
            } else {
                free(tmp);
                return obj;                              // return the last object
            }
        }

        // create/get the next object along the path
        if(cJSON_HasObjectItem(obj, token)) {
            obj = cJSON_GetObjectItemCaseSensitive(obj, token);
        } else {
            obj = cJSON_AddObjectToObject(obj, token);
        }
    }
    return NULL;
}

SETSTATUS Settings::setint(const char *key, const int32_t value) {
    if(strlen(key) <= 0) {
        return SETSTATUS::DOES_NOT_EXIST;
    }

    cJSON *obj;
    obj = m_createPathAndGetLast(key);
    if(obj == NULL) {
        return SETSTATUS::DOES_NOT_EXIST;
    }

    char *tmp = (char *)malloc(strlen(key) + 1);

    char *token;
    char *lastToken = NULL;
    memcpy(tmp, key, strlen(key) + 1);

    token = strtok(tmp, "/");
    while(token != NULL) {
        lastToken = token;
        token = strtok(NULL, "/");
    }

    cJSON_AddNumberToObject(obj, lastToken, value);
    free(tmp);
    saveToDisk();
    return SETSTATUS::OK;
}

SETSTATUS Settings::setString(const char *key, const char *value) {
    if(strlen(key) <= 0) {
        return SETSTATUS::DOES_NOT_EXIST;
    }

    cJSON *obj;
    obj = m_createPathAndGetLast(key);
    if(obj == NULL) {
        return SETSTATUS::DOES_NOT_EXIST;
    }

    char *tmp = (char *)malloc(strlen(key) + 1);

    char *token;
    char *lastToken = NULL;
    memcpy(tmp, key, strlen(key) + 1);

    token = strtok(tmp, "/");
    while(token != NULL) {
        lastToken = token;
        token = strtok(NULL, "/");
    }

    cJSON *ret = cJSON_AddStringToObject(obj, lastToken, value);
    free(tmp);
    saveToDisk();
    return SETSTATUS::OK;
}
SETSTATUS Settings::setBool(const char *key, const bool value) {
    if(strlen(key) <= 0) {
        return SETSTATUS::DOES_NOT_EXIST;
    }

    cJSON *obj;
    obj = m_createPathAndGetLast(key);
    if(obj == NULL) {
        return SETSTATUS::DOES_NOT_EXIST;
    }

    char *tmp = (char *)malloc(strlen(key) + 1);
    char *token;
    char *lastToken = NULL;
    memcpy(tmp, key, strlen(key) + 1);

    token = strtok(tmp, "/");
    while(token != NULL) {
        lastToken = token;
        token = strtok(NULL, "/");
    }

    cJSON_AddBoolToObject(obj, lastToken, value);
    free(tmp);
    saveToDisk();
    return SETSTATUS::OK;
}

SETSTATUS Settings::setDouble(const char *key, const double value) {
    if(strlen(key) <= 0) {
        return SETSTATUS::DOES_NOT_EXIST;
    }

    cJSON *obj;
    obj = m_createPathAndGetLast(key);
    if(obj == NULL) {
        return SETSTATUS::DOES_NOT_EXIST;
    }

    char *tmp = (char *)malloc(strlen(key) + 1);

    char *token;
    char *lastToken = NULL;
    memcpy(tmp, key, strlen(key) + 1);

    token = strtok(tmp, "/");
    while(token != NULL) {
        lastToken = token;
        token = strtok(NULL, "/");
    }

    cJSON_AddNumberToObject(obj, lastToken, value);
    free(tmp);
    saveToDisk();
    return SETSTATUS::OK;
}

// free with wifi_connection_t_delete
SETSTATUS Settings::getWifiConnections(wifi_connection_t **data, uint16_t *data_len) {
    *data = (wifi_connection_t *)calloc(1, sizeof(wifi_connection_t));
    if(*data == NULL) {
        ulog_fatal("Allocation Error");
        return SETSTATUS::ALLOCATION_ERROR;
    }
    *data_len = 0;
    bool dataGood = true;
    cJSON *networks = cJSON_CreateObject();
    cJSON *net = NULL;
    SETSTATUS ret = m_getobjat("wifi_connections", networks);

    if(ret != SETSTATUS::OK) {
        return SETSTATUS::DOES_NOT_EXIST;
    }

    cJSON_ArrayForEach(net, networks) {
        wifi_connection_t tmp_data;
        dataGood = true;
        cJSON *ssid = cJSON_GetObjectItemCaseSensitive(net, "ssid");

        if(cJSON_IsString(ssid)) {
            tmp_data.ssid = (char *)malloc(strlen(ssid->valuestring) + 1);
            if(tmp_data.ssid == NULL) {
                ulog_fatal("Allocation Error");
                return SETSTATUS::ALLOCATION_ERROR;
            }
            strcpy(tmp_data.ssid, ssid->valuestring);
        } else {
            dataGood = false;
        }

        cJSON *passwd = cJSON_GetObjectItemCaseSensitive(net, "passwd");
        if(cJSON_IsString(passwd)) {
            tmp_data.passwd = (char *)malloc(strlen(passwd->valuestring) + 1);
            strcpy(tmp_data.passwd, passwd->valuestring);
            if(tmp_data.passwd == NULL) {
                free(tmp_data.ssid);
                ulog_fatal("Allocation Error");
                return SETSTATUS::ALLOCATION_ERROR;
            }
        } else {
            dataGood = false;
        }

        if(dataGood) {
            *data = (wifi_connection_t *)realloc(*data, sizeof(wifi_connection_t) * ((*data_len) + 1));
            if(*data == NULL) {
                wifi_connection_t_delete(&tmp_data);
                ulog_fatal("Allocation Error");
                return SETSTATUS::ALLOCATION_ERROR;
            }
            memcpy(*data + (*data_len), &tmp_data, sizeof(wifi_connection_t));
            (*data_len)++;
            // data[*data_len] = tmp_data;
        } else {
            wifi_connection_t_delete(&tmp_data);
        }
        memset(&tmp_data, 0, sizeof(wifi_connection_t));
    }

    return SETSTATUS::OK;
}

SETSTATUS Settings::addWifiConnection(const char *ssid, const char *passwd) {
    cJSON *nets;
    if(!cJSON_HasObjectItem(m_data, "wifi_connections")) {
        nets = cJSON_CreateArray();
    } else {
        nets = cJSON_GetObjectItem(m_data, "wifi_connections");
    }

    cJSON *net = cJSON_CreateObject();
    cJSON_AddStringToObject(net, "ssid", ssid);
    cJSON_AddStringToObject(net, "passwd", passwd);
    cJSON_AddItemToArray(nets, net);

    saveToDisk();
    return SETSTATUS::OK;
}

SETSTATUS Settings::deleteWifiConnection(const char *ssid) {
    cJSON *nets;
    if(cJSON_HasObjectItem(m_data, "wifi_connections")) {
        nets = cJSON_GetObjectItem(m_data, "wifi_connections");
    } else {
        return SETSTATUS::DOES_NOT_EXIST;
    }
    cJSON *net = NULL;
    cJSON *ssid_obj = NULL;
    uint16_t index = 0;
    cJSON_ArrayForEach(net, nets) {
        ssid_obj = cJSON_GetObjectItem(net, "ssid");
        if((strcmp(cJSON_GetStringValue(ssid_obj), ssid) == 0)) {
            cJSON_DeleteItemFromArray(nets, index);
            saveToDisk();
            return SETSTATUS::OK;
        }
        index++;
    }
    return SETSTATUS::DOES_NOT_EXIST;
}

SETSTATUS Settings::m_read(const char *path) {
    FIL fp;
    FRESULT fr;
    fr = f_open(&fp, path, FA_READ);
    if(fr != FR_OK) {
        ulog_warn("Error while opening %s\n%s", path, FRESULT_str(fr));
        return SETSTATUS::FILE_NOT_FOUND;
    }
    uint64_t size = f_size(&fp);
    UINT bytes_read = 0;
    char *buffer = (char *)calloc(size, 1);
    if(buffer == NULL) {
        ulog_fatal("Failed to allocate Buffer with %llu Bytes for JSON", size);
    }

    fr = f_read(&fp, buffer, size, &bytes_read);
    if(fr != FR_OK) {
        ulog_error("Error while reading %s\n%s", path, FRESULT_str(fr));
        free(buffer);
        return SETSTATUS::FILE_ERROR;
    }

    fr = f_close(&fp);
    if(fr != FR_OK) {
        ulog_error("Error while closing %s\n%s", path, FRESULT_str(fr));
        free(buffer);
        return SETSTATUS::FILE_ERROR;
    }

    m_data = cJSON_ParseWithLength(buffer, bytes_read);
    free(buffer);
    if(m_data == NULL) {
        ulog_error("Error While parsing JSON");
        return SETSTATUS::PARSING_ERROR;
    }
    return SETSTATUS::OK;
}

SETSTATUS Settings::m_write(const char *path) {
    FIL fp;
    FRESULT fr;
    char *tmp;

    fr = f_open(&fp, path, FA_WRITE | FA_OPEN_ALWAYS);
    if(fr != FR_OK) {
        ulog_error("Error while opening %s\n%s", path, FRESULT_str(fr));
        return SETSTATUS::FILE_NOT_FOUND;
    }

    UINT bytes_written = 0;
    tmp = cJSON_Print(m_data);
    fr = f_write(&fp, tmp, strlen(tmp), &bytes_written);
    if(fr != FR_OK) {
        ulog_error("Error while writing %s%s", path, FRESULT_str(fr));
        return SETSTATUS::FILE_ERROR;
    }
    ulog_info("Writing %d Bytes to File. %d Bytes were actually written.", strlen(tmp), bytes_written);
    free(tmp);

    fr = f_close(&fp);
    if(fr != FR_OK) {
        ulog_error("Error while closing %s\n%s", path, FRESULT_str(fr));
        return SETSTATUS::FILE_ERROR;
    }
    return SETSTATUS::OK;
}

void Settings::printjson() {
    char *s = cJSON_Print(m_data);
    ulog_debug("Settings Data: %s", s);
    free(s);
}

SETSTATUS Settings::saveToDisk() {
    return m_write(m_path);
}

bool Settings::m_check_loaded() {
    if(m_data == NULL) {
        return false;
    }
    return true;
}

void wifi_connection_t_delete(wifi_connection_t *data) {
    free(data->passwd);
    free(data->ssid);
}
