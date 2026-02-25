#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "config.h"
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define mkdir(path, mode) _mkdir(path)
#define access _access
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#pragma pack(push, 1)
// BMP 파일 헤더 구조체
typedef struct {
    uint16_t type;          // 파일 타입 (BM)
    uint32_t size;          // 파일 크기
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;        // 픽셀 데이터 시작 위치
} BMPFileHeader;

// BMP 정보 헤더 구조체
typedef struct {
    uint32_t size;          // 이 구조체의 크기
    int32_t width;          // 이미지 너비
    int32_t height;         // 이미지 높이
    uint16_t planes;        // 컬러 플레인 수
    uint16_t bitCount;      // 픽셀당 비트 수
    uint32_t compression;   // 압축 방식
    uint32_t imageSize;     // 이미지 데이터 크기
    int32_t xPixelsPerM;    // 가로 해상도
    int32_t yPixelsPerM;    // 세로 해상도
    uint32_t colorsUsed;    // 사용된 색상 수
    uint32_t colorsImportant; // 중요한 색상 수
} BMPInfoHeader;
#pragma pack(pop)

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
int read_bmp(const char *filename, RGBArray *rgb) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("파일을 열 수 없습니다: %s\n", filename);
        return -1;
    }

    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;

    // 파일 헤더 읽기
    if (fread(&fileHeader, sizeof(BMPFileHeader), 1, file) != 1) {
        printf("파일 헤더 읽기 실패\n");
        fclose(file);
        return -1;
    }

    // BMP 파일인지 확인
    if (fileHeader.type != 0x4D42) { // 'BM'
        printf("BMP 파일이 아닙니다\n");
        fclose(file);
        return -1;
    }

    // 정보 헤더 읽기
    if (fread(&infoHeader, sizeof(BMPInfoHeader), 1, file) != 1) {
        printf("정보 헤더 읽기 실패\n");
        fclose(file);
        return -1;
    }

    // 24비트 BMP만 지원
    if (infoHeader.bitCount != 24) {
        printf("24비트 BMP만 지원합니다. 현재: %d비트\n", infoHeader.bitCount);
        fclose(file);
        return -1;
    }

    int width = infoHeader.width;
    int height = abs(infoHeader.height); // 높이는 음수일 수 있음 (top-down)
    int isTopDown = infoHeader.height < 0;

    // RGB 배열 메모리 할당
    rgb->width = width;
    rgb->height = height;
    rgb->r = (uint8_t *)malloc(width * height * sizeof(uint8_t));
    rgb->g = (uint8_t *)malloc(width * height * sizeof(uint8_t));
    rgb->b = (uint8_t *)malloc(width * height * sizeof(uint8_t));

    if (!rgb->r || !rgb->g || !rgb->b) {
        printf("메모리 할당 실패\n");
        fclose(file);
        return -1;
    }

    // 픽셀 데이터로 이동
    fseek(file, fileHeader.offset, SEEK_SET);

    // 행당 바이트 수 계산 (4바이트 정렬)
    int rowSize = ((width * 3 + 3) / 4) * 4;

    // 픽셀 데이터 읽기
    uint8_t *rowBuffer = (uint8_t *)malloc(rowSize);
    if (!rowBuffer) {
        printf("행 버퍼 할당 실패\n");
        fclose(file);
        return -1;
    }

    for (int y = 0; y < height; y++) {
        size_t bytesRead = fread(rowBuffer, 1, rowSize, file);
        if (bytesRead != (size_t)rowSize) {
            printf("픽셀 데이터 읽기 실패\n");
            free(rowBuffer);
            fclose(file);
            return -1;
        }

        int yIndex = isTopDown ? y : (height - 1 - y); // bottom-up인 경우 역순

        for (int x = 0; x < width; x++) {
            int pixelIndex = yIndex * width + x;
            int bufferIndex = x * 3;

            // BMP는 BGR 순서로 저장됨
            rgb->b[pixelIndex] = rowBuffer[bufferIndex + 0];
            rgb->g[pixelIndex] = rowBuffer[bufferIndex + 1];
            rgb->r[pixelIndex] = rowBuffer[bufferIndex + 2];
        }
    }

    free(rowBuffer);
    fclose(file);
    return 0;
}

// RGB 배열을 RGBG 배열로 변환
// RGBG 배열 구조:
//   Even row (y=0, 2, 4, ...): [R, G], [G, B], [R, G], [G, B] ...
//   Odd row  (y=1, 3, 5, ...): [B, G], [R, G], [B, G], [R, G] ...
// 각 픽셀은 2바이트로 저장됨
int rgb_to_rgbg(RGBArray *rgb, RGBGArray *rgbg) {
    rgbg->width = rgb->width;
    rgbg->height = rgb->height;
    
    // RGBG 패턴: 각 픽셀을 2바이트로 저장
    // Even row: [R, G], [G, B] 반복
    // Odd row:  [B, G], [R, G] 반복
    int size = rgb->width * rgb->height * 2; // 각 픽셀당 2바이트
    rgbg->data = (uint8_t *)malloc(size);
    
    if (!rgbg->data) {
        printf("RGBG 배열 메모리 할당 실패\n");
        return -1;
    }

    for (int y = 0; y < rgb->height; y++) {
        for (int x = 0; x < rgb->width; x++) {
            int pixelIndex = y * rgb->width + x;
            int rgbgIndex = pixelIndex * 2; // 각 픽셀당 2바이트

            if (y % 2 == 0) {
                // Even row: [R, G], [G, B] 패턴
                if (x % 2 == 0) {
                    // 짝수 열: [R, G]
                    rgbg->data[rgbgIndex + 0] = rgb->r[pixelIndex];
                    rgbg->data[rgbgIndex + 1] = rgb->g[pixelIndex];
                } else {
                    // 홀수 열: [G, B]
                    rgbg->data[rgbgIndex + 0] = rgb->g[pixelIndex];
                    rgbg->data[rgbgIndex + 1] = rgb->b[pixelIndex];
                }
            } else {
                // Odd row: [B, G], [R, G] 패턴
                if (x % 2 == 0) {
                    // 짝수 열: [B, G]
                    rgbg->data[rgbgIndex + 0] = rgb->b[pixelIndex];
                    rgbg->data[rgbgIndex + 1] = rgb->g[pixelIndex];
                } else {
                    // 홀수 열: [R, G]
                    rgbg->data[rgbgIndex + 0] = rgb->r[pixelIndex];
                    rgbg->data[rgbgIndex + 1] = rgb->g[pixelIndex];
                }
            }
        }
    }

    return 0;
}

// RGB 배열 해제
void free_rgb(RGBArray *rgb) {
    if (rgb) {
        free(rgb->r);
        free(rgb->g);
        free(rgb->b);
        rgb->r = NULL;
        rgb->g = NULL;
        rgb->b = NULL;
    }
}

// RGBG 배열 해제
void free_rgbg(RGBGArray *rgbg) {
    if (rgbg) {
        free(rgbg->data);
        rgbg->data = NULL;
    }
}

// RGB 배열 정보 출력
void print_rgb_info(RGBArray *rgb, const char *filename) {
    printf("파일: %s\n", filename);
    printf("크기: %d x %d\n", rgb->width, rgb->height);
    printf("첫 번째 픽셀 (R, G, B): (%d, %d, %d)\n", 
           rgb->r[0], rgb->g[0], rgb->b[0]);
    printf("마지막 픽셀 (R, G, B): (%d, %d, %d)\n", 
           rgb->r[rgb->width * rgb->height - 1], 
           rgb->g[rgb->width * rgb->height - 1], 
           rgb->b[rgb->width * rgb->height - 1]);
}

// RGBG 배열 정보 출력
void print_rgbg_info(RGBGArray *rgbg, const char *filename) {
    printf("RGBG 배열 - 파일: %s\n", filename);
    printf("크기: %d x %d\n", rgbg->width, rgbg->height);
    printf("첫 번째 픽셀 (2바이트): [%d, %d]\n", 
           rgbg->data[0], rgbg->data[1]);
    if (rgbg->width > 1) {
        printf("두 번째 픽셀 (2바이트): [%d, %d]\n", 
               rgbg->data[2], rgbg->data[3]);
    }
}

// 디렉토리 생성 함수 (재귀적)
static int create_directory(const char *path) {
    char temp[1024];
    char *p = NULL;
    size_t len;

    snprintf(temp, sizeof(temp), "%s", path);
    len = strlen(temp);
    
    // 경로 끝의 구분자 제거
    if (len > 0 && (temp[len - 1] == '/' || temp[len - 1] == '\\')) {
        temp[len - 1] = 0;
        len--;
    }
    
    // Windows와 Unix 경로 구분자 모두 처리
    for (p = temp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            char separator = *p;
            *p = 0;
            
            // 디렉토리가 존재하지 않으면 생성
#ifdef _WIN32
            if (_access(temp, 0) != 0) {
                if (_mkdir(temp) != 0) {
                    return -1;
                }
            }
#else
            if (access(temp, 0) != 0) {
                if (mkdir(temp, 0755) != 0) {
                    return -1;
                }
            }
#endif
            *p = separator;
        }
    }
    
    // 최종 디렉토리 생성
#ifdef _WIN32
    if (_access(temp, 0) != 0) {
        if (_mkdir(temp) != 0) {
            return -1;
        }
    }
#else
    if (access(temp, 0) != 0) {
        if (mkdir(temp, 0755) != 0) {
            return -1;
        }
    }
#endif
    
    return 0;
}

// RGBG 배열을 10bit PPM 파일로 저장
int save_rgbg_to_ppm_10bit(RGBGArray *rgbg, const char *filename) {
    if (!rgbg || !rgbg->data) {
        printf("유효하지 않은 RGBG 배열\n");
        return -1;
    }

    // 디렉토리 생성
    char dir_path[1024];
    strncpy(dir_path, filename, sizeof(dir_path) - 1);
    dir_path[sizeof(dir_path) - 1] = '\0';
    
    char *last_slash = strrchr(dir_path, '/');
    if (!last_slash) {
        last_slash = strrchr(dir_path, '\\');
    }
    if (last_slash) {
        *last_slash = '\0';
        if (create_directory(dir_path) != 0) {
            printf("디렉토리 생성 실패: %s\n", dir_path);
            return -1;
        }
    }

    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("파일을 생성할 수 없습니다: %s\n", filename);
        return -1;
    }

    // PPM 헤더 작성 (P6 형식, 10bit = 최대값 1023)
    // RGBG 배열을 RGB 3채널 형식으로 변환하여 저장
    // Even row: [R, G, 0], [0, G, B]
    // Odd row:  [0, G, B], [R, G, 0]
    fprintf(file, "P6\n%d %d\n1023\n", rgbg->width, rgbg->height);

    // RGBG 배열을 RGB 3채널 형식으로 변환하여 저장
    for (int y = 0; y < rgbg->height; y++) {
        for (int x = 0; x < rgbg->width; x++) {
            int pixelIndex = y * rgbg->width + x;
            int rgbgIndex = pixelIndex * 2; // 각 픽셀당 2바이트

            // RGBG 배열에서 2바이트 가져오기
            uint8_t ch1 = rgbg->data[rgbgIndex + 0];
            uint8_t ch2 = rgbg->data[rgbgIndex + 1];

            // RGB 3채널로 변환
            uint8_t r, g, b;
            if (y % 2 == 0) {
                // Even row: [R, G, 0], [0, G, B]
                if (x % 2 == 0) {
                    // 짝수 열: [R, G] → [R, G, 0]
                    r = ch1; // R
                    g = ch2; // G
                    b = 0;   // 0
                } else {
                    // 홀수 열: [G, B] → [0, G, B]
                    r = 0;   // 0
                    g = ch1; // G
                    b = ch2; // B
                }
            } else {
                // Odd row: [0, G, B], [R, G, 0]
                if (x % 2 == 0) {
                    // 짝수 열: [B, G] → [0, G, B]
                    r = 0;   // 0
                    g = ch2; // G
                    b = ch1; // B
                } else {
                    // 홀수 열: [R, G] → [R, G, 0]
                    r = ch1; // R
                    g = ch2; // G
                    b = 0;   // 0
                }
            }

            // 8bit (0-255)를 10bit (0-1023)로 확장
            // 공식: 10bit_value = (8bit_value * 1023) / 255
            uint16_t r_10bit = (uint16_t)((r * 1023) / 255);
            uint16_t g_10bit = (uint16_t)((g * 1023) / 255);
            uint16_t b_10bit = (uint16_t)((b * 1023) / 255);

            // 빅엔디안으로 16bit 값 저장 (PPM은 빅엔디안)
            uint8_t r_high = (r_10bit >> 8) & 0xFF;
            uint8_t r_low = r_10bit & 0xFF;
            uint8_t g_high = (g_10bit >> 8) & 0xFF;
            uint8_t g_low = g_10bit & 0xFF;
            uint8_t b_high = (b_10bit >> 8) & 0xFF;
            uint8_t b_low = b_10bit & 0xFF;

            // RGB 순서로 저장 (각 채널당 2바이트)
            fwrite(&r_high, 1, 1, file);
            fwrite(&r_low, 1, 1, file);
            fwrite(&g_high, 1, 1, file);
            fwrite(&g_low, 1, 1, file);
            fwrite(&b_high, 1, 1, file);
            fwrite(&b_low, 1, 1, file);
        }
    }

    fclose(file);
    printf("10bit PPM 파일 저장 완료: %s\n", filename);
    return 0;
}

// RGBG 배열을 12bit PPM 파일로 저장
int save_rgbg_to_ppm_12bit(RGBGArray *rgbg, const char *filename) {
    if (!rgbg || !rgbg->data) {
        printf("유효하지 않은 RGBG 배열\n");
        return -1;
    }

    // 디렉토리 생성
    char dir_path[1024];
    strncpy(dir_path, filename, sizeof(dir_path) - 1);
    dir_path[sizeof(dir_path) - 1] = '\0';
    
    char *last_slash = strrchr(dir_path, '/');
    if (!last_slash) {
        last_slash = strrchr(dir_path, '\\');
    }
    if (last_slash) {
        *last_slash = '\0';
        if (create_directory(dir_path) != 0) {
            printf("디렉토리 생성 실패: %s\n", dir_path);
            return -1;
        }
    }

    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("파일을 생성할 수 없습니다: %s\n", filename);
        return -1;
    }

    // PPM 헤더 작성 (P6 형식, 12bit = 최대값 4095)
    // RGBG 배열을 RGB 3채널 형식으로 변환하여 저장
    // Even row: [R, G, 0], [0, G, B]
    // Odd row:  [0, G, B], [R, G, 0]
    fprintf(file, "P6\n%d %d\n4095\n", rgbg->width, rgbg->height);

    // RGBG 배열을 RGB 3채널 형식으로 변환하여 저장
    for (int y = 0; y < rgbg->height; y++) {
        for (int x = 0; x < rgbg->width; x++) {
            int pixelIndex = y * rgbg->width + x;
            int rgbgIndex = pixelIndex * 2; // 각 픽셀당 2바이트

            // RGBG 배열에서 2바이트 가져오기
            uint8_t ch1 = rgbg->data[rgbgIndex + 0];
            uint8_t ch2 = rgbg->data[rgbgIndex + 1];

            // RGB 3채널로 변환
            uint8_t r, g, b;
            if (y % 2 == 0) {
                // Even row: [R, G, 0], [0, G, B]
                if (x % 2 == 0) {
                    // 짝수 열: [R, G] → [R, G, 0]
                    r = ch1; // R
                    g = ch2; // G
                    b = 0;   // 0
                } else {
                    // 홀수 열: [G, B] → [0, G, B]
                    r = 0;   // 0
                    g = ch1; // G
                    b = ch2; // B
                }
            } else {
                // Odd row: [0, G, B], [R, G, 0]
                if (x % 2 == 0) {
                    // 짝수 열: [B, G] → [0, G, B]
                    r = 0;   // 0
                    g = ch2; // G
                    b = ch1; // B
                } else {
                    // 홀수 열: [R, G] → [R, G, 0]
                    r = ch1; // R
                    g = ch2; // G
                    b = 0;   // 0
                }
            }

            // 8bit (0-255)를 12bit (0-4095)로 확장
            // 공식: 12bit_value = (8bit_value * 4095) / 255
            uint16_t r_12bit = (uint16_t)((r * 4095) / 255);
            uint16_t g_12bit = (uint16_t)((g * 4095) / 255);
            uint16_t b_12bit = (uint16_t)((b * 4095) / 255);

            // 빅엔디안으로 16bit 값 저장 (PPM은 빅엔디안)
            uint8_t r_high = (r_12bit >> 8) & 0xFF;
            uint8_t r_low = r_12bit & 0xFF;
            uint8_t g_high = (g_12bit >> 8) & 0xFF;
            uint8_t g_low = g_12bit & 0xFF;
            uint8_t b_high = (b_12bit >> 8) & 0xFF;
            uint8_t b_low = b_12bit & 0xFF;

            // RGB 순서로 저장 (각 채널당 2바이트)
            fwrite(&r_high, 1, 1, file);
            fwrite(&r_low, 1, 1, file);
            fwrite(&g_high, 1, 1, file);
            fwrite(&g_low, 1, 1, file);
            fwrite(&b_high, 1, 1, file);
            fwrite(&b_low, 1, 1, file);
        }
    }

    fclose(file);
    printf("12bit PPM 파일 저장 완료: %s\n", filename);
    return 0;
}

// RGB 배열을 8bit BMP 파일로 저장
int save_rgb_to_bmp_8bit(RGBArray *rgb, const char *filename) {
    if (!rgb || !rgb->r || !rgb->g || !rgb->b) {
        printf("유효하지 않은 RGB 배열\n");
        return -1;
    }

    // 입력 파일명에서 파일명만 추출
    const char *base_name = strrchr(filename, '/');
    if (!base_name) {
        base_name = strrchr(filename, '\\');
    }
    if (base_name) {
        base_name++; // 슬래시 다음부터
    } else {
        base_name = filename; // 경로가 없으면 전체를 파일명으로 사용
    }

    // test_out/img 경로로 저장 경로 생성
    char output_path[1024];
    snprintf(output_path, sizeof(output_path), "test_out/img/%s", base_name);

    // test_out/img 디렉토리 생성
    if (create_directory("test_out/img") != 0) {
        printf("디렉토리 생성 실패: test_out/img\n");
        return -1;
    }

    FILE *file = fopen(output_path, "wb");
    if (!file) {
        printf("파일을 생성할 수 없습니다: %s\n", output_path);
        return -1;
    }

    int width = rgb->width;
    int height = rgb->height;

    // 행당 바이트 수 계산 (4바이트 정렬)
    int rowSize = ((width + 3) / 4) * 4;
    int imageSize = rowSize * height;
    int paletteSize = 256 * 4; // 256색 * 4바이트 (BGR + reserved)

    // 팔레트 생성 (6-6-6 비트 양자화를 사용하여 256색 생성)
    // 6-6-6 비트 = 64*64*64 = 262144 색상 중에서 균등하게 256색 선택
    uint8_t palette[256][4]; // B, G, R, reserved
    
    // 더 나은 팔레트: 6-6-6 비트 양자화를 사용하되, 256색을 균등하게 분포
    // 방법: 각 채널을 6레벨로 양자화 (0-63), 총 216색을 사용하고 나머지는 보간
    int paletteIndex = 0;
    
    // 6-6-6 비트 양자화: 각 채널을 6단계로 나눔 (0, 51, 102, 153, 204, 255)
    // 총 6*6*6 = 216색을 먼저 생성
    for (int r_level = 0; r_level < 6 && paletteIndex < 256; r_level++) {
        for (int g_level = 0; g_level < 6 && paletteIndex < 256; g_level++) {
            for (int b_level = 0; b_level < 6 && paletteIndex < 256; b_level++) {
                palette[paletteIndex][2] = (r_level * 255) / 5; // R
                palette[paletteIndex][1] = (g_level * 255) / 5; // G
                palette[paletteIndex][0] = (b_level * 255) / 5; // B
                palette[paletteIndex][3] = 0;                   // reserved
                paletteIndex++;
            }
        }
    }
    
    // 나머지 40색을 채우기 위해 추가 색상 생성 (회색 톤 등)
    while (paletteIndex < 256) {
        int gray = (paletteIndex - 216) * 255 / (256 - 216);
        palette[paletteIndex][2] = gray; // R
        palette[paletteIndex][1] = gray; // G
        palette[paletteIndex][0] = gray; // B
        palette[paletteIndex][3] = 0;    // reserved
        paletteIndex++;
    }

    // RGB 값을 팔레트 인덱스로 변환하는 함수
    // 가장 가까운 팔레트 색상을 찾음
    uint8_t *indexData = (uint8_t *)malloc(width * height);
    if (!indexData) {
        printf("인덱스 데이터 메모리 할당 실패\n");
        fclose(file);
        return -1;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int pixelIndex = y * width + x;
            uint8_t r = rgb->r[pixelIndex];
            uint8_t g = rgb->g[pixelIndex];
            uint8_t b = rgb->b[pixelIndex];

            // 가장 가까운 팔레트 색상 찾기
            int minDist = 256 * 256 * 256;
            int bestIndex = 0;

            for (int i = 0; i < 256; i++) {
                int dr = (int)r - (int)palette[i][2];
                int dg = (int)g - (int)palette[i][1];
                int db = (int)b - (int)palette[i][0];
                int dist = dr * dr + dg * dg + db * db;

                if (dist < minDist) {
                    minDist = dist;
                    bestIndex = i;
                }
            }

            indexData[pixelIndex] = bestIndex;
        }
    }

    // BMP 파일 헤더 작성
    BMPFileHeader fileHeader;
    fileHeader.type = 0x4D42; // 'BM'
    fileHeader.size = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + paletteSize + imageSize;
    fileHeader.reserved1 = 0;
    fileHeader.reserved2 = 0;
    fileHeader.offset = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + paletteSize;

    // BMP 정보 헤더 작성
    BMPInfoHeader infoHeader;
    infoHeader.size = sizeof(BMPInfoHeader);
    infoHeader.width = width;
    infoHeader.height = height; // 양수 = bottom-up
    infoHeader.planes = 1;
    infoHeader.bitCount = 8;
    infoHeader.compression = 0; // BI_RGB
    infoHeader.imageSize = imageSize;
    infoHeader.xPixelsPerM = 0;
    infoHeader.yPixelsPerM = 0;
    infoHeader.colorsUsed = 256;
    infoHeader.colorsImportant = 256;

    // 헤더 쓰기
    fwrite(&fileHeader, sizeof(BMPFileHeader), 1, file);
    fwrite(&infoHeader, sizeof(BMPInfoHeader), 1, file);

    // 팔레트 쓰기
    fwrite(palette, 1, paletteSize, file);

    // 픽셀 데이터 쓰기 (bottom-up, 행은 역순)
    uint8_t *rowBuffer = (uint8_t *)malloc(rowSize);
    if (!rowBuffer) {
        printf("행 버퍼 할당 실패\n");
        free(indexData);
        fclose(file);
        return -1;
    }

    for (int y = height - 1; y >= 0; y--) {
        // 행 데이터 복사
        for (int x = 0; x < width; x++) {
            int pixelIndex = y * width + x;
            rowBuffer[x] = indexData[pixelIndex];
        }
        // 패딩 추가
        for (int x = width; x < rowSize; x++) {
            rowBuffer[x] = 0;
        }
        fwrite(rowBuffer, 1, rowSize, file);
    }

    free(rowBuffer);
    free(indexData);
    fclose(file);
    printf("8bit BMP 파일 저장 완료: %s\n", output_path);
    return 0;
}

// 256GRAY.bmp 파일을 이용한 RGB, RGBG 배열 변환 테스트 함수
int test_256gray_bmp(void) {
    const char *filename = "test_img/1080x2392/256GRAY.bmp";
    RGBArray rgb;
    RGBGArray rgbg;

    printf("========================================\n");
    printf("256GRAY.bmp 변환 테스트 시작\n");
    printf("========================================\n\n");

    // BMP 파일 읽기
    printf("[1/4] BMP 파일 읽는 중: %s\n", filename);
    if (read_bmp(filename, &rgb) != 0) {
        printf("오류: BMP 파일 읽기 실패\n");
        return -1;
    }
    printf("✓ BMP 파일 읽기 성공\n\n");

    // RGB 배열 정보 출력
    printf("[2/4] RGB 배열 정보:\n");
    print_rgb_info(&rgb, filename);
    printf("\n");

    // RGBG 배열 생성
    printf("[3/4] RGBG 배열 생성 중...\n");
    if (rgb_to_rgbg(&rgb, &rgbg) != 0) {
        printf("오류: RGBG 배열 생성 실패\n");
        free_rgb(&rgb);
        return -1;
    }
    printf("✓ RGBG 배열 생성 성공\n\n");

    // RGBG 배열 정보 출력
    printf("[4/4] RGBG 배열 정보:\n");
    print_rgbg_info(&rgbg, filename);
    printf("\n");

    // 샘플 픽셀 값 출력 (처음 5개, 중간 5개, 마지막 5개)
    printf("RGB 배열 샘플 픽셀 값:\n");
    int total_pixels = rgb.width * rgb.height;
    int sample_indices[] = {0, 1, 2, 3, 4, 
                            total_pixels / 2, total_pixels / 2 + 1, total_pixels / 2 + 2,
                            total_pixels - 3, total_pixels - 2, total_pixels - 1};
    int num_samples = sizeof(sample_indices) / sizeof(sample_indices[0]);
    
    for (int i = 0; i < num_samples; i++) {
        int idx = sample_indices[i];
        if (idx < total_pixels) {
            printf("  픽셀[%d]: R=%3d, G=%3d, B=%3d\n", 
                   idx, rgb.r[idx], rgb.g[idx], rgb.b[idx]);
        }
    }
    printf("\n");

    printf("RGBG 배열 샘플 픽셀 값:\n");
    for (int i = 0; i < num_samples; i++) {
        int idx = sample_indices[i];
        if (idx < total_pixels) {
            int rgbg_idx = idx * 2; // 각 픽셀당 2바이트
            int y = idx / rgb.width;
            int x = idx % rgb.width;
            printf("  픽셀[%d] (%d,%d): [%3d, %3d]", 
                   idx, y, x,
                   rgbg.data[rgbg_idx], 
                   rgbg.data[rgbg_idx + 1]);
            // 패턴 설명
            if (y % 2 == 0) {
                if (x % 2 == 0) {
                    printf(" → Even row, 짝수 열: [R, G]");
                } else {
                    printf(" → Even row, 홀수 열: [G, B]");
                }
            } else {
                if (x % 2 == 0) {
                    printf(" → Odd row, 짝수 열: [B, G]");
                } else {
                    printf(" → Odd row, 홀수 열: [R, G]");
                }
            }
            printf("\n");
        }
    }
    printf("\n");

    // 메모리 해제
    free_rgb(&rgb);
    free_rgbg(&rgbg);

    printf("========================================\n");
    printf("테스트 완료!\n");
    printf("========================================\n");
    return 0;
}

// RGBG 배열 구조 확인 함수 ([R, G], [B, G] 형태인지 검증)
int verify_rgbg_structure(void) {
    const char *filename = "test_img/1080x2392/256GRAY.bmp";
    RGBArray rgb;
    RGBGArray rgbg;

    printf("========================================\n");
    printf("RGBG 배열 구조 확인 테스트\n");
    printf("========================================\n\n");

    // BMP 파일 읽기
    printf("[1/3] BMP 파일 읽는 중: %s\n", filename);
    if (read_bmp(filename, &rgb) != 0) {
        printf("오류: BMP 파일 읽기 실패\n");
        return -1;
    }
    printf("✓ BMP 파일 읽기 성공 (크기: %d x %d)\n\n", rgb.width, rgb.height);

    // RGBG 배열 생성
    printf("[2/3] RGBG 배열 생성 중...\n");
    if (rgb_to_rgbg(&rgb, &rgbg) != 0) {
        printf("오류: RGBG 배열 생성 실패\n");
        free_rgb(&rgb);
        return -1;
    }
    printf("✓ RGBG 배열 생성 성공\n\n");

    // RGBG 배열 구조 확인 (2x2 그리드 패턴 확인)
    printf("[3/3] RGBG 배열 구조 확인:\n");
    printf("예상 패턴 (2x2 그리드):\n");
    printf("  Even row (y=0): [R, G]  [G, B]  [R, G]  [G, B] ...\n");
    printf("  Odd row  (y=1): [B, G]  [R, G]  [B, G]  [R, G] ...\n\n");

    // 처음 4개 픽셀 (2x2 그리드) 확인
    printf("처음 2x2 그리드 픽셀 값:\n");
    for (int y = 0; y < 2 && y < rgb.height; y++) {
        for (int x = 0; x < 2 && x < rgb.width; x++) {
            int pixelIndex = y * rgb.width + x;
            int rgbgIndex = pixelIndex * 2; // 각 픽셀당 2바이트
            
            printf("  위치 (%d, %d):\n", y, x);
            printf("    원본 RGB: R=%3d, G=%3d, B=%3d\n", 
                   rgb.r[pixelIndex], rgb.g[pixelIndex], rgb.b[pixelIndex]);
            printf("    RGBG 배열: [%3d, %3d]\n",
                   rgbg.data[rgbgIndex + 0],
                   rgbg.data[rgbgIndex + 1]);
            
            // 패턴 확인
            if (y % 2 == 0) {
                if (x % 2 == 0) {
                    printf("    → Even row, 짝수 열: [R, G] 형태 ✓\n");
                } else {
                    printf("    → Even row, 홀수 열: [G, B] 형태 ✓\n");
                }
            } else {
                if (x % 2 == 0) {
                    printf("    → Odd row, 짝수 열: [B, G] 형태 ✓\n");
                } else {
                    printf("    → Odd row, 홀수 열: [R, G] 형태 ✓\n");
                }
            }
            printf("\n");
        }
    }

    // 실제 저장된 값 확인 (더 많은 샘플)
    printf("실제 저장된 값 (처음 10개 픽셀):\n");
    for (int i = 0; i < 10 && i < rgb.width * rgb.height; i++) {
        int rgbgIndex = i * 2; // 각 픽셀당 2바이트
        int y = i / rgb.width;
        int x = i % rgb.width;
        
        printf("  픽셀[%d] (%d,%d): [%3d, %3d]",
               i, y, x,
               rgbg.data[rgbgIndex + 0],
               rgbg.data[rgbgIndex + 1]);
        
        if (y % 2 == 0) {
            if (x % 2 == 0) {
                printf(" → Even row, 짝수 열: [R, G]");
            } else {
                printf(" → Even row, 홀수 열: [G, B]");
            }
        } else {
            if (x % 2 == 0) {
                printf(" → Odd row, 짝수 열: [B, G]");
            } else {
                printf(" → Odd row, 홀수 열: [R, G]");
            }
        }
        printf("\n");
    }

    // 메모리 해제
    free_rgb(&rgb);
    free_rgbg(&rgbg);

    printf("\n========================================\n");
    printf("구조 확인 완료!\n");
    printf("========================================\n");
    return 0;
}

// save_rgbg_to_ppm_10bit, save_rgbg_to_ppm_12bit 함수 테스트
int test_ppm_save_functions(void) {
    const char *filename = "test_img/1080x2392/256GRAY.bmp";
    RGBArray rgb;
    RGBGArray rgbg;

    printf("========================================\n");
    printf("PPM 저장 함수 테스트 시작\n");
    printf("========================================\n\n");

    // BMP 파일 읽기
    printf("[1/5] BMP 파일 읽는 중: %s\n", filename);
    if (read_bmp(filename, &rgb) != 0) {
        printf("오류: BMP 파일 읽기 실패\n");
        return -1;
    }
    printf("✓ BMP 파일 읽기 성공 (크기: %d x %d)\n\n", rgb.width, rgb.height);

    // RGBG 배열 생성
    printf("[2/5] RGBG 배열 생성 중...\n");
    if (rgb_to_rgbg(&rgb, &rgbg) != 0) {
        printf("오류: RGBG 배열 생성 실패\n");
        free_rgb(&rgb);
        return -1;
    }
    printf("✓ RGBG 배열 생성 성공\n\n");

    // 샘플 픽셀 값 확인
    printf("[3/5] 샘플 픽셀 값 확인:\n");
    int sample_idx = 0;
    int rgbg_idx = sample_idx * 2; // 각 픽셀당 2바이트
    int y = sample_idx / rgb.width;
    int x = sample_idx % rgb.width;
    printf("  원본 RGB[0] (%d,%d): R=%3d, G=%3d, B=%3d\n", 
           y, x, rgb.r[sample_idx], rgb.g[sample_idx], rgb.b[sample_idx]);
    printf("  RGBG[0] (%d,%d): [%3d, %3d]", y, x,
           rgbg.data[rgbg_idx], rgbg.data[rgbg_idx + 1]);
    if (y % 2 == 0) {
        if (x % 2 == 0) {
            printf(" → Even row, 짝수 열: [R, G]\n");
        } else {
            printf(" → Even row, 홀수 열: [G, B]\n");
        }
    } else {
        if (x % 2 == 0) {
            printf(" → Odd row, 짝수 열: [B, G]\n");
        } else {
            printf(" → Odd row, 홀수 열: [R, G]\n");
        }
    }
    printf("\n");

    // 10bit PPM 저장 테스트
    const char *ppm_10bit_file = "test_out/img/test_256GRAY_10bit.ppm";
    printf("[4/5] 10bit PPM 파일 저장 테스트: %s\n", ppm_10bit_file);
    if (save_rgbg_to_ppm_10bit(&rgbg, ppm_10bit_file) != 0) {
        printf("오류: 10bit PPM 저장 실패\n");
        free_rgb(&rgb);
        free_rgbg(&rgbg);
        return -1;
    }
    printf("✓ 10bit PPM 저장 성공\n\n");

    // 12bit PPM 저장 테스트
    const char *ppm_12bit_file = "test_out/img/test_256GRAY_12bit.ppm";
    printf("[5/5] 12bit PPM 파일 저장 테스트: %s\n", ppm_12bit_file);
    if (save_rgbg_to_ppm_12bit(&rgbg, ppm_12bit_file) != 0) {
        printf("오류: 12bit PPM 저장 실패\n");
        free_rgb(&rgb);
        free_rgbg(&rgbg);
        return -1;
    }
    printf("✓ 12bit PPM 저장 성공\n\n");

    // 저장된 파일 검증 (파일 크기 확인)
    FILE *file;
    long file_size;
    
    file = fopen(ppm_10bit_file, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        fclose(file);
        // PPM 헤더 + 픽셀 데이터 (각 픽셀당 6바이트: R 2바이트 + G 2바이트 + B 2바이트)
        long expected_size = 15 + (rgbg.width * rgbg.height * 6); // 헤더 약 15바이트 + 픽셀 데이터
        printf("  10bit PPM 파일 크기: %ld bytes (예상: 약 %ld bytes, RGB 3채널)\n", 
               file_size, expected_size);
    }

    file = fopen(ppm_12bit_file, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        fclose(file);
        long expected_size = 15 + (rgbg.width * rgbg.height * 6);
        printf("  12bit PPM 파일 크기: %ld bytes (예상: 약 %ld bytes, RGB 3채널)\n", 
               file_size, expected_size);
    }

    // 저장된 파일의 헤더 확인
    printf("\n저장된 PPM 파일 헤더 확인:\n");
    char header[64];
    file = fopen(ppm_10bit_file, "rb");
    if (file) {
        if (fgets(header, sizeof(header), file)) {
            printf("  10bit PPM 첫 줄: %s", header);
        }
        if (fgets(header, sizeof(header), file)) {
            printf("  10bit PPM 크기: %s", header);
        }
        if (fgets(header, sizeof(header), file)) {
            printf("  10bit PPM 최대값: %s", header);
        }
        fclose(file);
    }

    file = fopen(ppm_12bit_file, "rb");
    if (file) {
        if (fgets(header, sizeof(header), file)) {
            printf("  12bit PPM 첫 줄: %s", header);
        }
        if (fgets(header, sizeof(header), file)) {
            printf("  12bit PPM 크기: %s", header);
        }
        if (fgets(header, sizeof(header), file)) {
            printf("  12bit PPM 최대값: %s", header);
        }
        fclose(file);
    }

    // 저장된 PPM 파일의 패턴 확인 (처음 몇 개 픽셀)
    printf("\n저장된 PPM 파일 패턴 확인 (처음 4개 픽셀):\n");
    printf("예상 패턴:\n");
    printf("  Even row (y=0): [R, G, 0]  [0, G, B]  [R, G, 0]  [0, G, B] ...\n");
    printf("  Odd row  (y=1): [0, G, B]  [R, G, 0]  [0, G, B]  [R, G, 0] ...\n\n");

    file = fopen(ppm_10bit_file, "rb");
    if (file) {
        // 헤더 건너뛰기
        if (fgets(header, sizeof(header), file) == NULL) goto cleanup_header; // P6
        if (fgets(header, sizeof(header), file) == NULL) goto cleanup_header; // 크기
        if (fgets(header, sizeof(header), file) == NULL) goto cleanup_header; // 최대값
        
        // 처음 4개 픽셀 읽기 (각 픽셀당 RGB 3채널 * 2바이트 = 6바이트)
        for (int i = 0; i < 4 && i < rgbg.width * rgbg.height; i++) {
            int y = i / rgbg.width;
            int x = i % rgbg.width;
            uint8_t r_bytes[2], g_bytes[2], b_bytes[2];
            
            if (fread(r_bytes, 1, 2, file) != 2) break;
            if (fread(g_bytes, 1, 2, file) != 2) break;
            if (fread(b_bytes, 1, 2, file) != 2) break;
            
            // 빅엔디안으로 10bit 값 읽기
            uint16_t r_10bit = (r_bytes[0] << 8) | r_bytes[1];
            uint16_t g_10bit = (g_bytes[0] << 8) | g_bytes[1];
            uint16_t b_10bit = (b_bytes[0] << 8) | b_bytes[1];
            
            // 10bit를 8bit로 변환 (확인용)
            uint8_t r = (r_10bit * 255) / 1023;
            uint8_t g = (g_10bit * 255) / 1023;
            uint8_t b = (b_10bit * 255) / 1023;
            
            printf("  픽셀[%d] (%d,%d): [R=%3d, G=%3d, B=%3d]", i, y, x, r, g, b);
            
            if (y % 2 == 0) {
                if (x % 2 == 0) {
                    printf(" → Even row, 짝수 열: [R, G, 0]");
                    if (b == 0) printf(" ✓");
                } else {
                    printf(" → Even row, 홀수 열: [0, G, B]");
                    if (r == 0) printf(" ✓");
                }
            } else {
                if (x % 2 == 0) {
                    printf(" → Odd row, 짝수 열: [0, G, B]");
                    if (r == 0) printf(" ✓");
                } else {
                    printf(" → Odd row, 홀수 열: [R, G, 0]");
                    if (b == 0) printf(" ✓");
                }
            }
            printf("\n");
        }
        cleanup_header:
        fclose(file);
    }

    // 메모리 해제
    free_rgb(&rgb);
    free_rgbg(&rgbg);

    printf("\n========================================\n");
    printf("PPM 저장 함수 테스트 완료!\n");
    printf("========================================\n");
    return 0;
}

int main(int argc, char *argv[]) {
    // config.txt 파일 로드
    if (load_config("config.txt") != 0) {
        printf("경고: config.txt 파일을 읽을 수 없습니다. 기본값을 사용합니다.\n");
    }
    
    // 테스트 모드: 인자가 없으면 256GRAY.bmp 테스트 실행
    if (argc == 1) {
        return test_256gray_bmp();
    }
    
    // 테스트 모드: --test-ppm 옵션으로 PPM 저장 함수 테스트
    if (argc == 2 && strcmp(argv[1], "--test-ppm") == 0) {
        return test_ppm_save_functions();
    }
    
    // 테스트 모드: --verify-rgbg 옵션으로 RGBG 구조 확인
    if (argc == 2 && strcmp(argv[1], "--verify-rgbg") == 0) {
        return verify_rgbg_structure();
    }
    
    // 테스트 모드: --print-config 옵션으로 config 값 출력
    if (argc == 2 && strcmp(argv[1], "--print-config") == 0) {
        print_config();
        return 0;
    }
    
    if (argc < 2) {
        printf("사용법: %s [옵션] [<BMP 파일 경로>]\n", argv[0]);
        printf("  인자 없음: test_img/1080x2392/256GRAY.bmp 테스트 실행\n");
        printf("  --test-ppm: PPM 저장 함수 테스트 실행\n");
        printf("  --verify-rgbg: RGBG 배열 구조 확인\n");
        printf("  --print-config: config 값 출력\n");
        printf("  인자 있음: 지정한 BMP 파일 처리\n");
        printf("예시:\n");
        printf("  %s\n", argv[0]);
        printf("  %s --test-ppm\n", argv[0]);
        printf("  %s --verify-rgbg\n", argv[0]);
        printf("  %s --print-config\n", argv[0]);
        printf("  %s test_img/1080x2392/CT_W.bmp\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    RGBArray rgb;
    RGBGArray rgbg;

    // BMP 파일 읽기
    printf("BMP 파일 읽는 중: %s\n", filename);
    if (read_bmp(filename, &rgb) != 0) {
        return 1;
    }

    // RGB 배열 정보 출력
    print_rgb_info(&rgb, filename);

    // RGBG 배열 생성
    printf("\nRGBG 배열 생성 중...\n");
    if (rgb_to_rgbg(&rgb, &rgbg) != 0) {
        free_rgb(&rgb);
        return 1;
    }

    // RGBG 배열 정보 출력
    print_rgbg_info(&rgbg, filename);

    // RGBG 배열을 PPM 파일로 저장 (예시)
    // 실제 사용 시에는 원하는 파일명으로 변경하세요
    char output_filename_10bit[512];
    char output_filename_12bit[512];
    char *base_name = strrchr(filename, '/');
    if (!base_name) base_name = strrchr(filename, '\\');
    if (base_name) base_name++;
    else base_name = (char *)filename;
    
    // 확장자 제거
    char name_without_ext[256];
    strncpy(name_without_ext, base_name, sizeof(name_without_ext) - 1);
    name_without_ext[sizeof(name_without_ext) - 1] = '\0';
    char *dot = strrchr(name_without_ext, '.');
    if (dot) *dot = '\0';
    
    snprintf(output_filename_10bit, sizeof(output_filename_10bit), 
             "test_out/img/%.200s_10bit.ppm", name_without_ext);
    snprintf(output_filename_12bit, sizeof(output_filename_12bit), 
             "test_out/img/%.200s_12bit.ppm", name_without_ext);
    
    printf("\nPPM 파일 저장 중...\n");
    save_rgbg_to_ppm_10bit(&rgbg, output_filename_10bit);
    save_rgbg_to_ppm_12bit(&rgbg, output_filename_12bit);

    // RGB 배열을 8bit BMP로 저장
    char output_filename_8bit[512];
    snprintf(output_filename_8bit, sizeof(output_filename_8bit), 
             "test_out/img/%.200s_8bit.bmp", name_without_ext);
    printf("\n8bit BMP 파일 저장 중...\n");
    save_rgb_to_bmp_8bit(&rgb, output_filename_8bit);

    // 메모리 해제
    free_rgb(&rgb);
    free_rgbg(&rgbg);

    printf("\n완료!\n");
    return 0;
}
