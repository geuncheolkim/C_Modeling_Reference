#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// int nor_vol_grn[8] 배열 선언 및 초기화 방법들

int main(void) {
    // 방법 1: 배열 선언 및 초기화 (모든 값 지정)
    int nor_vol_grn1[8] = {232, 209, 184, 163, 156, 149, 137, 130};
    
    // 방법 2: 배열 선언 후 나중에 값 할당
    int nor_vol_grn2[8];
    nor_vol_grn2[0] = 232;
    nor_vol_grn2[1] = 209;
    nor_vol_grn2[2] = 184;
    nor_vol_grn2[3] = 163;
    nor_vol_grn2[4] = 156;
    nor_vol_grn2[5] = 149;
    nor_vol_grn2[6] = 137;
    nor_vol_grn2[7] = 130;
    
    // 방법 3: 반복문으로 값 할당
    int nor_vol_grn3[8];
    int values[] = {232, 209, 184, 163, 156, 149, 137, 130};
    for (int i = 0; i < 8; i++) {
        nor_vol_grn3[i] = values[i];
    }
    
    // 방법 4: memset으로 0으로 초기화 후 값 할당
    int nor_vol_grn4[8];
    memset(nor_vol_grn4, 0, sizeof(nor_vol_grn4));
    // 이후 필요한 값 할당
    
    // 방법 5: 부분 초기화 (나머지는 0으로 자동 초기화)
    int nor_vol_grn5[8] = {232, 209, 184};  // 처음 3개만 초기화, 나머지는 0
    
    // 배열 출력 예제
    printf("=== nor_vol_grn1 배열 ===\n");
    for (int i = 0; i < 8; i++) {
        printf("nor_vol_grn1[%d] = %d\n", i, nor_vol_grn1[i]);
    }
    
    printf("\n=== nor_vol_grn2 배열 ===\n");
    for (int i = 0; i < 8; i++) {
        printf("nor_vol_grn2[%d] = %d\n", i, nor_vol_grn2[i]);
    }
    
    printf("\n=== nor_vol_grn3 배열 ===\n");
    for (int i = 0; i < 8; i++) {
        printf("nor_vol_grn3[%d] = %d\n", i, nor_vol_grn3[i]);
    }
    
    // 배열 크기 확인
    printf("\n배열 크기: %zu bytes\n", sizeof(nor_vol_grn1));
    printf("배열 요소 개수: %zu\n", sizeof(nor_vol_grn1) / sizeof(nor_vol_grn1[0]));
    
    return 0;
}
