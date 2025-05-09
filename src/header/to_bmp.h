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
    USIN image_size;   // �摜�T�C�Y
    USIN image_width;  // �摜��
    USIN image_height; // �摜��
    USCH bit_depth;    // �r�b�g�[�x�i�F�[�x�j
    USCH color_type;   // �J���[�^�C�v
    USCH press;        // ���k���@
    USCH filter;       // �t�B���^�[���@
    USCH display;      // �C���^���[�X���@�i�\�����@�j
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BMPFILEHEADER {
    USSH bfType;          // �t�@�C���^�C�v ('BM'�Œ�)
    USIN bfSize;          // �t�@�C���S�̂̃T�C�Y
    USSH bfReserved1;     // �\��̈�i�g�p���Ȃ��j
    USSH bfReserved2;     // �\��̈�i�g�p���Ȃ��j
    USIN bfOffset;        // �p���b�g�̊J�n�ʒu
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BMPINFOHEADER {
    USIN biSize;          // ���w�b�_�[�̃T�C�Y (�ʏ�40)
    USIN biWidth;         // �摜�̕�
    USIN biHeight;        // �摜�̍���
    USSH biPlanes;        // �v���[�����i���1�j
    USSH biBitCount;      // �F�[�x�i�r�b�g���j
    USIN biCompression;   // ���k�`���i�ʏ�0���񈳏k�j
    USIN biSizeImage;     // �摜�f�[�^�̃T�C�Y�i���k���j
    USIN biXPelsPerMeter; // �����𑜓x�i�s�N�Z��/m�j
    USIN biYPelsPerMeter; // �����𑜓x�i�s�N�Z��/m�j
    USIN biClrUsed;       // �g�p�����F��
    USIN biClrImportant;  // �d�v�ȐF��
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