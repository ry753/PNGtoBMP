#pragma once
/*================================================================================
include, header
================================================================================*/
#include"define_type.h"
#include<stdio.h>
#include<list>

/*================================================================================
namespace
================================================================================*/
using namespace std;

/*================================================================================
struct
================================================================================*/
#pragma pack(push, 1)
struct PNGDATA {
    USIN image_size;   // 画像サイズ
    USIN image_width;  // 画像幅
    USIN image_height; // 画像高
    USCH bit_depth;    // ビット深度（色深度）
    USCH color_type;   // カラータイプ
    USCH press;        // 圧縮方法
    USCH filter;       // フィルター方法
    USCH display;      // インタレース方法（表示方法）
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BMPFILEHEADER {
    USSH bfType;          // ファイルタイプ ('BM'固定)
    USIN bfSize;          // ファイル全体のサイズ
    USSH bfReserved1;     // 予約領域（使用しない）
    USSH bfReserved2;     // 予約領域（使用しない）
    USIN bfOffset;        // パレットの開始位置
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BMPINFOHEADER {
    USIN biSize;          // 情報ヘッダーのサイズ (通常40)
    USIN biWidth;         // 画像の幅
    USIN biHeight;        // 画像の高さ
    USSH biPlanes;        // プレーン数（常に1）
    USSH biBitCount;      // 色深度（ビット数）
    USIN biCompression;   // 圧縮形式（通常0＝非圧縮）
    USIN biSizeImage;     // 画像データのサイズ（圧縮時）
    USIN biXPelsPerMeter; // 水平解像度（ピクセル/m）
    USIN biYPelsPerMeter; // 垂直解像度（ピクセル/m）
    USIN biClrUsed;       // 使用される色数
    USIN biClrImportant;  // 重要な色数
};
#pragma pack(pop)

/*================================================================================
class
================================================================================*/
class TO_BMP {

public:

	// function
	INTE _exe_();

private:

	// function
	INTE _check_format_   (FILE* input_file_ptr);
	INTE _get_info_	      (FILE* input_file_ptr, PNGDATA& png_info);
	INTE _check_info_     (PNGDATA& png_info);
	INTE _get_idat_       (USCH* png_ptr, list<USCH*>& ptr_list, list<USIN>& size_list);
	INTE _link_idat_      (USCH* idat_ptr, list<USCH*>& ptr_list, list<USCH*>::iterator ptr_ite, list<USIN>& size_list, list<USIN>::iterator size_ite);
	INTE _unzip_idat_     (USCH* idat_ptr, INTE idat_size, USCH* unzip_ptr, INTE unzip_size);
	INTE _scanline_decode_(USCH* unzip_ptr, PNGDATA& png_info);
	INTE _delete_scanline_(USCH* unzip_ptr, USCH* rgb_ptr, INTE data_size, PNGDATA& png_info);
	INTE _premulti_alpha_ (USCH* rgb_ptr, INTE bmp_size);
	INTE _rgb_to_bgr_     (USCH* rgb_ptr, INTE bmp_size, PNGDATA& png_info);
	INTE _flip_side_up_   (USCH* rgb_ptr, USCH* bmp_ptr, PNGDATA& png_info);
	INTE _save_bmp_       (USCH* bmp_ptr, PNGDATA& png_info, INTE bmp_size);

};