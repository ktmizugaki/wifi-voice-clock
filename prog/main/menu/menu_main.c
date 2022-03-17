#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <esp_idf_version.h>
#include <esp_system.h>
#include <esp_ota_ops.h>
#include <esp_log.h>

#include <vcc.h>

#include "liblistview.h"
#include "libmenu.h"

#include "../gen/lang.h"
#include "../app_mode.h"

#include "menu.h"

#define TAG "menu"

static bool remote_maint(const menu_item_t *menu_item, unsigned int *full_update)
{
    app_mode_t ret =app_mode_settings();
    return ret==APP_MODE_SUSPEND;
}

static bool app_ver_value(listview_item_t *item,
    char *buf, size_t bufsize, bool full)
{
    const esp_app_desc_t *desc = esp_ota_get_app_description();
    snprintf(buf, bufsize, LANG_APP_VER_FMT, desc->version);
    return true;
}

static bool sdk_ver_value(listview_item_t *item,
    char *buf, size_t bufsize, bool full)
{
    const char *version = esp_get_idf_version();
    snprintf(buf, bufsize, LANG_SDK_VER_FMT, version);
    return true;
}

static bool chip_value(listview_item_t *item,
    char *buf, size_t bufsize, bool full)
{
    char tmp[34];
    esp_chip_info_t info;
    esp_chip_info(&info);
    snprintf(tmp, sizeof(tmp), "%dcores, rev %d%s%s%s",
        info.cores, info.revision,
        (info.features & CHIP_FEATURE_WIFI_BGN) ? ", WiFi": "",
        (info.features & CHIP_FEATURE_BT) ? ", BT": "",
        (info.features & CHIP_FEATURE_BLE) ? ", BLE" : "");
    snprintf(buf, bufsize, LANG_CHIP_FMT, tmp);
    return true;
}

static const char *CHARGE_STATE_STRS[] = {
    [VCC_CHARG_DISCONNECTED] = LANG_VCC_CHARG_DISCONNECTED,
    [VCC_CHARG_CHARGING] = LANG_VCC_CHARG_CHARGING,
    [VCC_CHARG_COMPLETED] = LANG_VCC_CHARG_COMPLETED,
    [3] = LANG_VCC_CHARG_INVALID,
};

static bool charge_state_value(listview_item_t *item,
    char *buf, size_t bufsize, bool full)
{
    vcc_charge_state_t state;
    if (vcc_get_charge_state(&state) != ESP_OK) {
        state = 3;
    }
    snprintf(buf, bufsize, LANG_VCC_CHARG_FMT, CHARGE_STATE_STRS[state]);
    return true;
}

static bool vcc_value(listview_item_t *item,
    char *buf, size_t bufsize, bool full)
{
    int vcc;
    if (vcc_read(&vcc, false) != ESP_OK) {
        strlcpy(buf, LANG_VCC_ERROR, bufsize);
        return true;
    }
    snprintf(buf, bufsize, LANG_VCC_FMT, vcc/1000, vcc%1000);
    return true;
}

static bool sysinfo(const menu_item_t *menu_item, unsigned int *full_update)
{
    listview_item_t items[] = {
        { LVIF_VALUE_TYPE_FUNC, { .get_value = charge_state_value }, NULL, NULL },
        { LVIF_VALUE_TYPE_FUNC, { .get_value = vcc_value }, NULL, NULL },
        { LVIF_VALUE_TYPE_FUNC, { .get_value = app_ver_value }, NULL, NULL },
        { LVIF_VALUE_TYPE_FUNC, { .get_value = sdk_ver_value }, NULL, NULL },
        { LVIF_VALUE_TYPE_FUNC, { .get_value = chip_value }, NULL, NULL },
    };
    listview_t list = {
        .flags = LVF_ITEMS_TYPE_ARRAY,
        .item_count = sizeof(items) / sizeof(items[0]),
        .items = items,
        .title = LANG_SYSINFO,
    };
    return listview(&list);
}

static const menu_item_t mi_remote_maint = {
    .flags = MIF_VALUE_TYPE_STR|MIF_EXEC_FUNC,
    .label = LANG_REMOTE_MAINT,
    .value = { .str = LANG_REMOTE_MAINT },
    .exec = { .func = remote_maint },
};
static const menu_item_t mi_sysinfo = {
    .flags = MIF_VALUE_TYPE_STR|MIF_EXEC_FUNC,
    .label = LANG_SYSINFO,
    .value = { .str = LANG_SYSINFO },
    .exec = { .func = sysinfo },
};
MAKE_MENU(static, m_main_menu,
    LANG_MENU_MAIN,
    &mi_remote_maint,
    &mi_sysinfo,
);

enum menu_result menu_main(void)
{
    return menu_run(&m_main_menu);
}
