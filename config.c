#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"
#include "config_tables.h" // 룩업 테이블 Include

// 전역 변수 메모리 할당
IpPorts_t ip_ports;
int regmap[REG_PAGE][PAGE_ADDR];

// 문자열 앞뒤 공백 제거 유틸리티 함수
static void trim_space(char *str) {
    char *p = str;
    int l = strlen(p);
    while (l > 0 && isspace(p[l - 1])) p[--l] = 0;
    while (*p && isspace(*p)) ++p, --l;
    memmove(str, p, l + 1);
}

// config.txt 로드 함수
int load_config(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: Cannot open config file '%s'.\n", filename);
        return -1;
    }

    char line[256];
    int line_num = 0;

    printf("========================================\n");
    printf("Config 파싱 시작: %s\n", filename);
    printf("========================================\n");

    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        trim_space(line);

        // 빈 줄이거나 주석(#, //)은 무시
        if (line[0] == '\0' || line[0] == '#' || (line[0] == '/' && line[1] == '/')) {
            continue;
        }

        char *eq_ptr = strchr(line, '=');
        if (eq_ptr) {
            *eq_ptr = '\0'; // '=' 위치를 널 문자로 변경하여 Key와 Value 분리
            char *key = line;
            char *val_str = eq_ptr + 1;

            trim_space(key);
            trim_space(val_str);

            // 10진수와 16진수(0x) 자동 판별하여 변환
            int value = (int)strtol(val_str, NULL, 0);
            int found = 0;

            // ========================================================
            // DBV 14bit port 통합 처리 (14bit -> H/L)
            // ========================================================
            if (strcmp(key, "por_dbv") == 0) {
                ip_ports.set_dbv_h = (value >> 8) & 0x3F; // 상위 6비트
                ip_ports.set_dbv_l = value & 0xFF;        // 하위 8비트
                
                printf("  [PORT-V] %-25s = %d -> (H: 0x%02X, L: 0x%02X)\n", 
                       key, value, ip_ports.set_dbv_h, ip_ports.set_dbv_l);
                found = 1; // 이미 처리했으므로 아래 검색 루프를 건너뜀
            }
            // ========================================================
             
            // ========================================================
            // 1. PORT 테이블 검색
            // ========================================================
            for (int i = 0; i < port_table_size; i++) {
                if (strcmp(port_table[i].name, key) == 0) {
                    *(port_table[i].ptr) = value;
                    printf("  [PORT] %-25s = %d (0x%X)\n", key, value, value);
                    found = 1;
                    break;
                }
            }
            // ======================================================
            
            // ========================================================
            // 2. PORT에 없으면 REG 테이블 검색
            // ========================================================
            if (!found) {
                for (int i = 0; i < reg_table_size; i++) {
                    if (strcmp(reg_table[i].name, key) == 0) {
                        *(reg_table[i].ptr) = value;
                        printf("  [REG ] %-25s = %d (0x%X)\n", key, value, value);
                        found = 1;
                        break;
                    }
                }
            }
            // ========================================================
            
            // ========================================================
            // 3. 둘 다 없으면 경고 출력 (오타 방지용)
            // ========================================================
            if (!found) {
                printf("  [WARN] Unknown key '%s' at line %d\n", key, line_num);
            }
            // ========================================================
        }
    }

    fclose(fp);
    printf("========================================\n\n");
    return 0;
}



// 현재 적용된 주요 설정값을 출력하는 함수
void print_config(void) {
    printf("--- Current Configuration ---\n");
    printf("  [PORT] por_rgbg_order       : %d\n", ip_ports.por_rgbg_order);
    printf("  [PORT] por_fcon             :%d\n", ip_ports.por_fcon);
    printf("  [PORT] por_dbv_h            : %d\n", ip_ports.por_dbv_h);
    printf("  [PORT] por_dbv_l            : %d\n", ip_ports.por_dbv_l);
    printf("  [PORT] por_freq_sel         : %d\n", ip_ports.por_freq_sel);
    printf("  [REG ] reg_ctc_lctc_en      : %d\n", regmap[0x05][0x01]);
    printf("  [REG ] reg_ctc_actc_en      : %d\n", regmap[0x05][0x02]);
    printf("  [REG ] reg_ctc_color_mode   : %d\n", regmap[0x05][0x03]);
    printf("  [REG ] reg_ctc_th_gray      : %d\n", regmap[0x05][0x08]);
    printf("  [REG ] reg_ctc_freq_gain0   : %d\n", regmap[0x05][0xD2]);
    printf("  [REG ] R_ctb_lctb_en        : %d\n", regmap[0x05][0x01]);
    printf("  [REG ] R_ctb_th_gray        : %d\n", regmap[0x05][0x08]);
    printf("-----------------------------\n");
}