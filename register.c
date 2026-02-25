// util_reg.c 내부 파서 로직 예시

#include "ip_top.h"
#include "reg_table.h" // 파이썬이 생성한 레지스터 테이블

IpExternalPorts_t ip_ports; // 실제 구조체 메모리 할당

// 파싱할 때 사용할 외부 포트 매핑 테이블
typedef struct {
    const char *name;
    int *ptr;
} PortMap;

static PortMap port_table[] = {
    {"por_rgbg_order",   &ip_ports.rgbg_order},
    {"por_dbv_h",        &ip_ports.dbv_h},
    {"por_dbv_l",        &ip_ports.dbv_l},
    {"por_fcon",         &ip_ports.fcon},
};
static int port_table_size = sizeof(port_table)/sizeof(port_table[0]);

// txt 로더 함수
void reg_load_txt(int (*regmap)[PAGE_ADDR], const char *filename) {
    FILE *fp = fopen(filename, "r");
    char line[256];
    
    while (fgets(line, sizeof(line), fp)) {
        // ... (주석 및 공백 제거 로직) ...
        
        char *key = /* 분리된 Key */;
        int value = /* 분리된 Value */;
        int found = 0;

        // 1. 먼저 외부 포트(Struct)인지 검사
        for(int i=0; i<port_table_size; i++) {
            if(strcmp(port_table[i].name, key) == 0) {
                *(port_table[i].ptr) = value;
                found = 1;
                break;
            }
        }
        
        // 2. 포트가 아니라면 내부 레지스터(regmap)인지 검사
        if(!found) {
            for (int i = 0; i < config_table_size; i++) {
                if (strcmp(config_table[i].name, key) == 0) {
                    *(config_table[i].ptr) = value;
                    break;
                }
            }
        }
    }
    fclose(fp);
}