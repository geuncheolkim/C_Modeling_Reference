#ifndef IMAGE_IO_H
#define IMAGE_IO_H

#include <stdint.h>

// RGB 배열 구조체
typedef struct {
    uint8_t *r;
    uint8_t *g;
    uint8_t *b;
    int width;
    int height;
} RGBArray;

// RGBG 배열 구조체
typedef struct {
    uint8_t *data;
    int width;
    int height;
} RGBGArray;

// BMP 파일 읽기 함수
// 반환값: 0 성공, -1 실패
int read_bmp(const char *filename, RGBArray *rgb);

// RGB 배열을 RGBG 배열로 변환
// 반환값: 0 성공, -1 실패
int rgb_to_rgbg(RGBArray *rgb, RGBGArray *rgbg);

// RGB 배열 메모리 해제
void free_rgb(RGBArray *rgb);

// RGBG 배열 메모리 해제
void free_rgbg(RGBGArray *rgbg);

// RGB 배열 정보 출력
void print_rgb_info(RGBArray *rgb, const char *filename);

// RGBG 배열 정보 출력
void print_rgbg_info(RGBGArray *rgbg, const char *filename);

// RGBG 배열을 10bit PPM 파일로 저장
// 반환값: 0 성공, -1 실패
int save_rgbg_to_ppm_10bit(RGBGArray *rgbg, const char *filename);

// RGBG 배열을 12bit PPM 파일로 저장
// 반환값: 0 성공, -1 실패
int save_rgbg_to_ppm_12bit(RGBGArray *rgbg, const char *filename);

// RGB 배열을 8bit BMP 파일로 저장
// 반환값: 0 성공, -1 실패
int save_rgb_to_bmp_8bit(RGBArray *rgb, const char *filename);

// 256GRAY.bmp 파일을 이용한 RGB, RGBG 배열 변환 테스트 함수
// 반환값: 0 성공, -1 실패
int test_256gray_bmp(void);

// save_rgbg_to_ppm_10bit, save_rgbg_to_ppm_12bit 함수 테스트
// 반환값: 0 성공, -1 실패
int test_ppm_save_functions(void);

// RGBG 배열 구조 확인 함수 ([R, G], [B, G] 형태인지 검증)
// 반환값: 0 성공, -1 실패
int verify_rgbg_structure(void);

#endif // IMAGE_IO_H
