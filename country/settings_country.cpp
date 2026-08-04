#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#define SETTINGS_CMD_GET  "settings get global wifi_country_code"
#define SETTINGS_CMD_PUT  "settings put global wifi_country_code JP"

int main() {
    FILE* fp = popen(SETTINGS_CMD_GET, "r");
    if (!fp) {
        return 1;
    }

    char result[64] = {0};
    if (fgets(result, sizeof(result), fp) != NULL) {
        size_t len = strlen(result);
        if (len > 0 && result[len - 1] == '\n') {
            result[len - 1] = '\0';
        }
        if (strlen(result) == 0 || strcmp(result, "null") == 0) {
            system(SETTINGS_CMD_PUT);
        }
    }
    pclose(fp);
    return 0;
}
