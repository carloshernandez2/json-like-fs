#include <cjson/cJSON.h>
#include <glib.h>

cJSON *cJSON_GetInObjectItemCaseSensitive(cJSON *object, gchar **string_list) {
    cJSON *item = object;
    while (*string_list) {
        item = cJSON_GetObjectItemCaseSensitive(item, *string_list);
        if (!item)
            return NULL;
        string_list++;
    }
    return item;
}

cJSON *cJSON_GetInObjectItemParentCaseSensitive(cJSON *object,
                                                gchar **string_list) {
    if (!object || !string_list[0])
        return object;

    cJSON *item = object;

    while (string_list[1]) {
        item = cJSON_GetObjectItemCaseSensitive(item, *string_list);
        if (!item)
            return NULL;
        string_list++;
    }

    return item;
}

cJSON *cJSON_CreateEmptyString() { return cJSON_CreateString(""); }
