#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// 1. 외부 입력 포트(PORT) 구조체 정의
// registers_list.csv에서 'Type'이 PORT인 항목들입니다.
typedef struct {
    int set_order_bgfirst_en;
    int set_dbv_h;
    int set_dbv_l;
    int set_freq_sel;
} IpPorts_t;

// 전역 인스턴스 선언 (다른 파일에서 ip_ports.set_freq_sel 형태로 접근 가능)
extern IpPorts_t ip_ports;

// IP 내부 레지스터 배열 선언 (기존 프로젝트에 맞게 크기 조절 가능)
#define REG_PAGE 16
#define PAGE_ADDR 256
extern int regmap[REG_PAGE][PAGE_ADDR];

// 함수 선언
int load_config(const char *filename);
void print_config(void);

#endif // CONFIG_H