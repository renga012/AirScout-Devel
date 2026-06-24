#ifndef SETTINGS_H
#define SETTINGS_H
#include <cJSON.h>
#include <stdint.h>

enum class SETSTATUS { OK, DOES_NOT_EXIST, WRONG_TYPE, FILE_NOT_FOUND, FILE_ERROR, PARSING_ERROR, ALLOCATION_ERROR };

typedef struct {
    char *ssid;
    char *passwd;
} wifi_connection_t;

void wifi_connection_t_delete(wifi_connection_t* data);

class Settings {
public:
    Settings(const char *path);
    ~Settings();
    SETSTATUS loadSettings();

    SETSTATUS getint(const char *key, int32_t *value);
    SETSTATUS getString(const char *key, char **strptr);
    SETSTATUS getBool(const char *key, bool *value);
    SETSTATUS getDouble(const char *key, double *value);

    SETSTATUS setint(const char *key, const int32_t value);
    SETSTATUS setString(const char *key, const char *value);
    SETSTATUS setBool(const char *key, const bool value);
    SETSTATUS setDouble(const char *key, const double value);

    SETSTATUS getWifiConnections(wifi_connection_t **data, uint16_t *data_len);
    SETSTATUS addWifiConnection(const char *ssid, const char *passwd);
    SETSTATUS deleteWifiConnection(const char *ssid);

    void printjson();
    SETSTATUS saveToDisk();

private:
    cJSON *m_data;
    char *m_path;

    SETSTATUS m_read(const char *path);
    SETSTATUS m_write(const char *path);
    SETSTATUS m_getobjat(const char *key, cJSON *obj);
    SETSTATUS m_loadDefaults();
    cJSON *m_createPathAndGetLast(const char *key);
    bool m_check_loaded();
};

#endif  // SETTINGS_H
