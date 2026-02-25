#ifndef CONFIG_TABLES_H
#define CONFIG_TABLES_H

#include "config.h"

// 문자열과 변수 포인터를 매핑할 구조체
typedef struct {
    const char *name;
    int *ptr;
} ConfigMap;

// PORT 매핑 테이블
static ConfigMap port_table[] = {
    {"por_rgbg_order",        &ip_ports.por_rgbg_order},
    {"por_dbv_h",             &ip_ports.por_dbv_h},
    {"por_dbv_l",             &ip_ports.por_dbv_l},
    {"por_fcon",              &ip_ports.por_fcon},
};
static const int port_table_size = sizeof(port_table) / sizeof(port_table[0]);

// REG 매핑 테이블 (regmap 배열과 직접 매핑)
static ConfigMap reg_table[] = {
    {"reg_ctc_lctc_en",           &regmap[0x05][0x01]},
    {"reg_ctc_actc_en",           &regmap[0x05][0x02]},
    {"reg_ctc_color_mode",        &regmap[0x05][0x03]},
    {"reg_ctc_th_gray",           &regmap[0x05][0x08]},
    {"reg_ctc_freq_gain0",        &regmap[0x05][0xD2]},
    // 필요에 따라 나머지 레지스터들도 여기에 추가...
};
static const int reg_table_size = sizeof(reg_table) / sizeof(reg_table[0]);

#endif // CONFIG_TABLES_H