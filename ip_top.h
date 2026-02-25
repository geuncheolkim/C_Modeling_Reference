// ip_top.h (또는 main.h)

// IP 외부에서 들어오는 Input Port / Parameter 모음
typedef struct {
    int rgbg_drder;
    int dbv_h;
    int dbv_l;
    int fcon;
} IpExternalPorts_t;

// 전역 인스턴스 선언 (extern)
extern IpExternalPorts_t ip_ports;