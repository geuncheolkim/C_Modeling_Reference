# BMP 파일 읽기 및 RGB/RGBG 배열 생성

이 프로그램은 BMP 파일을 읽어서 RGB 배열과 RGBG 배열을 생성합니다.

## 컴파일 방법

```bash
gcc -Wall -Wextra -O2 -o read_bmp read_bmp.c
```

또는 Makefile 사용:
```bash
make
```

## 사용 방법

```bash
./read_bmp <BMP 파일 경로>
```

예시:
```bash
./read_bmp test_img/1080x2392/CT_W.bmp
```

## 기능

1. **BMP 파일 읽기**: 24비트 BMP 파일을 읽어서 RGB 배열로 변환
2. **RGB 배열**: 각 픽셀의 R, G, B 값을 별도의 배열로 저장
3. **RGBG 배열**: RGBG 패턴으로 변환된 배열 생성

## 구조체

### RGBArray
- `r`, `g`, `b`: 각각 R, G, B 채널 데이터
- `width`, `height`: 이미지 크기

### RGBGArray
- `data`: RGBG 패턴 데이터 (각 픽셀당 4바이트)
- `width`, `height`: 이미지 크기

## 주의사항

- 24비트 BMP 파일만 지원합니다
- 메모리는 사용 후 반드시 `free_rgb()`와 `free_rgbg()`로 해제해야 합니다
