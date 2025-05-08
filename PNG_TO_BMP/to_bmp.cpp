/*================================================================================
header, function
================================================================================*/
#include"define_type.h"
#include"to_bmp.h"
#include"general.h"
#include"zlib.h" // DOCUMENT\zlib_license.txt
#include<stdio.h>
#include<stdlib.h>

/*================================================================================
namespace
================================================================================*/
using namespace std;

/*================================================================================
define
================================================================================*/
#define MAX_PATH		 260       // �t�@�C�����ő啶����
#define PATH_START		 6         // "input/"���l�������z��J�n�ʒu
#define MAX_FILE_SIZE	 10000000  // ���͉摜�̍ő�T�C�Y(10MB)
#define ID_SIZE			 8         // PNG���ʎq�T�C�Y
#define TO_WIDTH		 16        // png�̉����f�[�^�܂ł̃o�C�g��
#define TO_HEIGHT		 20        // png�̍����f�[�^�܂ł̃o�C�g��
#define TO_BITDEPTH		 24        // �F�[�x�܂ł̃o�C�g��
#define TO_COLORTYPE     25        // �J���[�^�C�v�܂ł̃o�C�g��
#define TO_PRESSMETHOD   26        // ���k�`���܂ł̃o�C�g��
#define TO_FILTERMETHOD  27        // �t�B���^�����܂ł̃o�C�g��
#define TO_INTERMETHOD   28        // �C���^���[�X�����܂ł̃o�C�g��
#define MAX_PIXEL        5000      // �摜���A�����̍ő�s�N�Z����
#define BIT_DEPTH        0x08      // ������r�b�g�[�x
#define COLOR_TYPE       0x06      // ������J���[�^�C�v
#define PRESS_METHOD     0x00      // �����鈳�k����
#define FILTERTYPE       0x00      // ������t�B���^����
#define INTERMETHOD      0x00      // ������C���^���[�X����
#define CHUNK_BITE       4         // IDAT, IEND��T������ۂɊm�F����o�C�g��
#define TO_CHUNKSIZE     8         // IDAT�`�����N�̎��ʎq����T�C�Y���擾����ۂɖ߂�o�C�g��
#define CHANNEL          4         // RGBA�̃`���l����
#define FILTERTYPESIZE   1         // �𓀌�f�[�^�̍s���Ƃ̐擪�ɕt�^�����t�B���^�^�C�v�̃o�C�g��

/*================================================================================
* _exe_() �N���X���֐��̎��s
================================================================================*/
INTE TO_BMP::_exe_() {

	FILE* input_file_ptr = NULL;
	CHAR  input_file_name[MAX_PATH] = "input/"; // ���̓t�@�C�����z��
	USCH  func_ret = 0;

	// �摜���
	struct PNGDATA png_info;
	memset(&png_info, 0, sizeof(struct PNGDATA));

	// =====================================================================
	// ���H�Ώۃt�@�C�����̎擾
	// ���݊m�F��A�t�H�[�}�b�g��摜���̌���
	// =====================================================================
	while (1) {

		// �t�@�C��������
		printf("Enter target file name. (e.g. image.png):");
		scanf_s("%s", &input_file_name[PATH_START], MAX_PATH - PATH_START);

		// ���݊m�F
		fopen_s(&input_file_ptr, input_file_name, "rb");
		if (input_file_ptr == NULL) {
			printf("File not found, Please re-enter.\n");
			continue;
		}

		// �t�@�C���t�H�[�}�b�g�̌���
		func_ret = _check_format_(input_file_ptr);
		if (func_ret != 0) { fclose(input_file_ptr); return 1; }

		// �摜���擾�A����
		_get_info_(input_file_ptr, png_info);
		func_ret = _check_info_(png_info);
		if (func_ret != 0) { fclose(input_file_ptr); return 1; }

		// ���؏����I��
		printf("Start processing...\n");
		break;

	}

	// png�̉摜�f�[�^�p���������m�ہAPNG�摜�f�[�^�擾
	USCH* png_first_ptr = (USCH*)malloc(png_info.image_size);
	if (png_first_ptr == NULL) {
		printf("Memory Error 1001\n");
		fclose(input_file_ptr);
		return 1;
	}

	// png�f�[�^���m��
	INTE read_ret = 0;
	read_ret = fread(png_first_ptr, png_info.image_size, 1, input_file_ptr);

	// �摜�t�@�C���J��
	fclose(input_file_ptr);

	// =====================================================================
	// IDAT�`�����N���擾�i�������̃|�C���^�̓��X�g�Ɋi�[�j
	// =====================================================================
	// �e�`�����N�f�[�^���i�[���ꂽ�������̐擪�A�h���X���X�g
	list<USCH*>           _FirstIdatPtrList; // IDAT�擪�|�C���^���X�g
	list<USCH*>::iterator _CPIte;            // ��L�C�e���[�^
	// �e�`�����N�̃T�C�Y�̃��X�g
	list<USIN>            _ChunkSizeList;    // IDAT�T�C�Y���X�g
	list<USIN>::iterator  _CSIte;            // ��L�C�e���[�^

	func_ret = _get_idat_(png_first_ptr, _FirstIdatPtrList, _ChunkSizeList);
	free(png_first_ptr);
	if (func_ret != 0) {
		for (_CPIte  = _FirstIdatPtrList.begin(); _CPIte != _FirstIdatPtrList.end(); _CPIte++) {
			free(*_CPIte);
		}
		_ChunkSizeList.clear();
		_FirstIdatPtrList.clear();
		return 1; 
	}

	// =====================================================================
	// IDAT�`�����N�̘A������
	// �A����T�C�Y�̌v�Z���A�A������IDAT�`�����N���i�[���郁�������m��
	// =====================================================================
	USIN idat_size = 0;
	for (_CSIte  = _ChunkSizeList.begin(); _CSIte != _ChunkSizeList.end(); _CSIte++) {
		idat_size += *_CSIte;
	}

	// �������m��
	USCH* idat_first_ptr = (USCH*)malloc(idat_size);
	if (idat_first_ptr == NULL) {
		printf("Memory Error 1003\n");
		for (_CPIte  = _FirstIdatPtrList.begin(); _CPIte != _FirstIdatPtrList.end(); _CPIte++) {
			free(*_CPIte);
		}
		_FirstIdatPtrList.clear();
		_ChunkSizeList.clear();
		return 1;
	}

	// �A�����s
	_link_idat_(idat_first_ptr, _FirstIdatPtrList, _CPIte, _ChunkSizeList, _CSIte);
	_FirstIdatPtrList.clear();
	_ChunkSizeList.clear();

	// =====================================================================
	// �A������IDAT�f�[�^���𓀂���
	// �𓀌�T�C�Y���v�Z���A�𓀌�f�[�^���i�[���郁�������m��
	// =====================================================================
	USIN unzip_size = ((png_info.image_width * CHANNEL) + FILTERTYPESIZE) * png_info.image_height;

	// �������m��
	USCH* unzip_first_ptr = (USCH*)malloc(unzip_size);
	if (unzip_first_ptr == NULL) {
		printf("Memory Error 1004\n");
		free(idat_first_ptr);
		return 1;
	}

	// �𓀎��s
	func_ret = _unzip_idat_(idat_first_ptr, idat_size, unzip_first_ptr, unzip_size);
	free(idat_first_ptr);
	if (func_ret != 0) {
		free(unzip_first_ptr);
		return 1; 
	}

	// =====================================================================
	// �𓀂��ē����X�L�������C���f�[�^��RGB�f�[�^�ɕ�������
	// =====================================================================
	func_ret = _scanline_decode_(unzip_first_ptr, png_info);
	if (func_ret != 0) {
		free(unzip_first_ptr);
		return 1;
	}

	// =====================================================================
	// �����_�̉𓀃f�[�^�ɂ�RGB�f�[�^�̂ق���
	// �X�L�������C�����܂܂�Ă��邽�߁A�������������
	// =====================================================================
	INTE data_size = (png_info.image_width * CHANNEL); // �P�s������̃t�B���^�^�C�v�����f�[�^�T�C�Y
	INTE rgb_size = data_size * png_info.image_height;

	// �t�B���^�^�C�v����������RGB�f�[�^���i�[���郁����
	USCH* rgb_first_ptr = (USCH*)malloc(rgb_size);
	if (rgb_first_ptr == NULL) {
		printf("Memory Error 1005\n");
		free(unzip_first_ptr);
		return 1;
	}

	// �������s
	func_ret = _delete_scanline_(unzip_first_ptr, rgb_first_ptr, data_size, png_info);
	free(unzip_first_ptr);
	if (func_ret != 0) {
		free(rgb_first_ptr);
		return 1;
	}

	// =====================================================================
	// �A���t�@�l��FF(����)�ł�RGB�f�[�^�������Ă��邽�߂�
	// BMP�ɏo�͂���ۂɂ��̐F���o�Ă��Ă��܂����Ƃ�����
	// ���̂��߁A�A���t�@�l��␳���鏈�����s��
	// =====================================================================
	INTE  bmp_size = png_info.image_width * png_info.image_height * CHANNEL;
	_premulti_alpha_(rgb_first_ptr, bmp_size);

	// =====================================================================
	// BMP�Ƀf�[�^���i�[���邷�邽�߂�RGB�f�[�^��BGR�f�[�^�ɕϊ�����
	// =====================================================================
	_rgb_to_bgr_(rgb_first_ptr, bmp_size, png_info);

	// =====================================================================
	// BMP�f�[�^�Ƃ��Ă��̂܂܏������ނ��߂Ɍ��݂�BGR�f�[�^���㉺���]����
	// bmp�f�[�^���i�[���郁�������m��
	// =====================================================================
	USCH* bmp_first_ptr = (USCH*)malloc(bmp_size);
	if (bmp_first_ptr == NULL) {
		printf("Memory Error 1006\n");
		free(rgb_first_ptr);
		return 1;
	}
	// �㉺���]�������s
	_flip_side_up_(rgb_first_ptr, bmp_first_ptr, png_info);

	// �������J��
	free(rgb_first_ptr);

	// =====================================================================
	// �ۑ�����
	// =====================================================================
	func_ret = _save_bmp_(bmp_first_ptr, png_info, bmp_size);
	if (func_ret != 0) { 
		free(bmp_first_ptr);
		return 1; 
	}
	free(bmp_first_ptr);

	// exe�I��
	return 0;
}

/*================================================================================
* _check_format_() ���͉摜��png�`�����𔻒�
================================================================================*/
INTE TO_BMP::_check_format_(FILE* input_file_ptr) {

	// �����|�C���^�̒��g�m�F
	if (input_file_ptr == NULL) { return 1; }

	// PNG���ʎq���i�[�����z��Ǝ擾�����擪�W�o�C�g���r
	USCH png_id [ID_SIZE] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
	USCH png_buf[ID_SIZE];
	USCH flg   = 0;
	INTE index = 0;

	// �z�񏉊��� 
	for (index = 0; index < ID_SIZE; index++) {
		png_buf[index] = 0;
	}

	// �擪�W�o�C�g�iPNG���ʎq�j�ǂݎ��
	INTE read_ret = 0;

	fseek(input_file_ptr, 0, SEEK_SET);
	fread(&png_buf[0], ID_SIZE, 1, input_file_ptr);
	fseek(input_file_ptr, 0, SEEK_SET);

	// �z��̒��g���r
	for (index = 0; index < 8; index++) {
		if (png_id[index] != png_buf[index]) {
			flg++;
		}
	}

	// ����
	if (flg == 0) {
		return 0;
	}
	else {
		printf("Please enter a png image.\n");
		return 1;
	}

}

/*================================================================================
* _get_info_() ���͉摜�̏����擾
================================================================================*/
INTE TO_BMP::_get_info_(FILE* input_file_ptr, PNGDATA& png_info) {

	// �����|�C���^�̒��g�m�F
	if (input_file_ptr == NULL) { return 1; };

	// �t�@�C���T�C�Y�擾
	fseek(input_file_ptr, 0, SEEK_END);
	png_info.image_size = ftell(input_file_ptr);
	fseek(input_file_ptr, 0, SEEK_SET);

	// IHDR�`�����N���̉摜�����擾
	INTE tmp4 = 0;     // �S�o�C�g�f�[�^�擾�p
	USCH tmp1 = 0;     // �P�o�C�g�f�[�^�擾�p

	// �摜��
	fseek(input_file_ptr, TO_WIDTH, SEEK_SET);
	fread(&tmp4, sizeof(tmp4), 1, input_file_ptr);
	png_info.image_width = endian_conv(tmp4);

	// �摜��
	fseek(input_file_ptr, TO_HEIGHT, SEEK_SET);
	fread(&tmp4, sizeof(tmp4), 1, input_file_ptr);
	png_info.image_height = endian_conv(tmp4);

	// �r�b�g�[�x
	fseek(input_file_ptr, TO_BITDEPTH, SEEK_SET);
	fread(&png_info.bit_depth, sizeof(tmp1), 1, input_file_ptr);

	// �J���[�^�C�v
	fseek(input_file_ptr, TO_COLORTYPE, SEEK_SET);
	fread(&png_info.color_type, sizeof(tmp1), 1, input_file_ptr);

	// ���k����
	fseek(input_file_ptr, TO_PRESSMETHOD, SEEK_SET);
	fread(&png_info.press, sizeof(tmp1), 1, input_file_ptr);

	// �t�B���^����
	fseek(input_file_ptr, TO_FILTERMETHOD, SEEK_SET);
	fread(&png_info.filter, sizeof(tmp1), 1, input_file_ptr);

	// �C���^���[�X����
	fseek(input_file_ptr, TO_INTERMETHOD, SEEK_SET);
	fread(&png_info.display, sizeof(tmp1), 1, input_file_ptr);

	return 0;

}

/*================================================================================
* _check_info_() ���͉摜�̏�������
================================================================================*/
INTE TO_BMP::_check_info_(PNGDATA& png_info) {

	// �摜�T�C�Y�̌��؁i�ő�10MB�j
	if (png_info.image_size > MAX_FILE_SIZE) {
		printf("Image size is too large.(MAX 10MB)\n");
		return 1;
	}
	// �摜���A�摜�����̍ő�px��5000x5000
	if (png_info.image_width > MAX_PIXEL || png_info.image_height > MAX_PIXEL) {
		printf("Image size is too large.(MAX 5000x5000px)\n");
		return 1;
	}
	// �r�b�g�[�x��0x08�̂��̌���
	if (png_info.bit_depth != BIT_DEPTH) {
		printf("The bit depth of this image is not supported.(only 0x08)\n");
		return 1;
	}
	// �J���[�^�C�v��0x06�̂��̌���
	if (png_info.color_type != COLOR_TYPE) {
		printf("The color type of this image is not supported.(only 0x06)\n");
		return 1;
	}
	// ���k������0x00�̂��̌���
	if (png_info.press != PRESS_METHOD) {
		printf("The compression method of this image is not supported.(only 0x00)\n");
		return 1;
	}
	// �t�B���^������0x00�̂��̌���
	if (png_info.filter != FILTERTYPE) {
		printf("%02x", png_info.filter);
		printf("The filter method of this image is not supported.(only 0x00)\n");
		return 1;
	}
	// �C���^���[�X������0x00�̂��̌���
	if (png_info.display != INTERMETHOD) {
		printf("The interlace method of this image is not supported.(only 0x00)\n");
		return 1;
	}

	return 0;
}

/*================================================================================
* _get_idat_() IDAT�`�����N���擾����
================================================================================*/
INTE TO_BMP::_get_idat_(USCH* png_ptr, list<USCH*>& ptr_list, list<USIN>& size_list) {

	// �����|�C���^�̒��g�m�F
	if (png_ptr == NULL) { return 1; };

	USCH iend_id[4] = { 0x49, 0x45, 0x4e, 0x44 }; // IEND�`�����N�̎��ʎq
	USCH idat_id[4] = { 0x49, 0x44, 0x41, 0x54 }; // IDAT�`�����N�̎��ʎq
	USCH search [4];
	USIN index = 0;

	// IEND�`�����N��������܂�IDAT�`�����N��T������
	// IDAT�`�����N������A�f�[�^���擾���A�T�C�Y�Ɛ擪�|�C���^���N���X�ϐ��̃��X�g�Ɋi�[
	while (1) {

		// search������ 
		for (index = 0; index < CHUNK_BITE; index++) {
			search[index] = 0;
		}

		// 4�o�C�g�擾
		for (index = 0; index < CHUNK_BITE; index++) {
			search[index] = *png_ptr;
			png_ptr++;
		}

		// IEND�`�����N�ŏI��
		if (search[0] == iend_id[0] && search[1] == iend_id[1] &&
			search[2] == iend_id[2] && search[3] == iend_id[3]) {
			break;
		}

		// IDAT�`�����N������AIDAT�̃f�[�^���擾
		// �擾�����f�[�^�T�C�Y�ƃf�[�^�̐擪�|�C���^�����X�g�Ɋi�[
		if (search[0] == idat_id[0] && search[1] == idat_id[1] &&
			search[2] == idat_id[2] && search[3] == idat_id[3]) {

			// �`�����N�T�C�Y���擾
			png_ptr -= TO_CHUNKSIZE;
			USIN idat_size = *((USIN*)png_ptr);
			idat_size = endian_conv(idat_size);

			size_list.push_back(idat_size);

			// �T�C�Y���擾�����̂ŁAIDAT�̐擪�֖߂�
			png_ptr += TO_CHUNKSIZE;

			// IDAT�T�C�Y���̃������m��
			USCH* idat_first_ptr = (USCH*)malloc(idat_size);
			if (idat_first_ptr == NULL) {
				printf("Memory Error 1002\n");
				return 1;
			}
			USCH* idat_ptr = idat_first_ptr; // ����p

			// IDAT�f�[�^�m��
			memcpy(idat_ptr, png_ptr, idat_size);
			png_ptr += idat_size + CHUNK_BITE;

			// �f�[�^�擾��A���X�g�ɐ擪�A�h���X���i�[���A�f�[�^�擾���I������
			ptr_list.push_back(idat_first_ptr);

		}
		else {
			// �P���X�^�[�g�����炵�Ȃ���T�����s��
			// 4�o�C�g�T�����A3�o�C�g�߂�...
			png_ptr -= 3;
		}

	}

	return 0;
}

/*================================================================================
* _link_idat_() IDAT�`�����N��A��
================================================================================*/
INTE TO_BMP::_link_idat_(USCH* idat_ptr, list<USCH*>& ptr_list, list<USCH*>::iterator ptr_ite, list<USIN>& size_list, list<USIN>::iterator size_ite) {

	// �����|�C���^�̒��g�m�F
	if (idat_ptr == NULL) { return 1; };

	for (ptr_ite = ptr_list.begin(), size_ite = size_list.begin();
		(ptr_ite) != ptr_list.end() && size_ite != size_list.end();
		(ptr_ite)++, size_ite++) {

		memcpy(idat_ptr, *ptr_ite, *size_ite);
		idat_ptr += *size_ite;

		free(*ptr_ite);

	}

	return 0;
}

/*================================================================================
* _unzip_idat_() IDAT�f�[�^����
================================================================================*/
INTE TO_BMP::_unzip_idat_(USCH* idat_ptr, INTE idat_size, USCH* unzip_ptr, INTE unzip_size) {

	// �����|�C���^�̒��g�m�F
	if (idat_ptr == NULL || unzip_ptr == NULL) { return 1; };

	// z_stream������
	z_stream stream;
	stream.zalloc   = Z_NULL;
	stream.zfree    = Z_NULL;
	stream.opaque   = Z_NULL;
	stream.next_in  = idat_ptr;  // IDAT�f�[�^�擪
	stream.avail_in = idat_size; // IDAT�f�[�^�T�C�Y

	// inflate������
	INTE ret = inflateInit(&stream);
	if (ret != Z_OK) {
		printf("init error: %d\n", ret);
		return 1;
	}

	// �\���̂ɉ𓀌�f�[�^�ڍׂ�ݒ�
	stream.next_out  = unzip_ptr;   // �𓀌�f�[�^�擪
	stream.avail_out = unzip_size;  // �𓀌�f�[�^�T�C�Y

	// ��
	ret = inflate(&stream, Z_FINISH);
	if (ret != Z_STREAM_END) {
		printf("unzip error: %d\n", ret);
		inflateEnd(&stream);
		return 1;
	}
	inflateEnd(&stream);

	return 0;
}

/*================================================================================
* _scanline_decode_() �𓀂���IDAT�f�[�^��RGB�ɕ���
================================================================================*/
INTE TO_BMP::_scanline_decode_(USCH* unzip_ptr, PNGDATA& png_info) {

	// �����|�C���^�̒��g�m�F
	if (unzip_ptr == NULL) { return 1; };

	// �ϐ��錾
	INTE  line_size = (png_info.image_width * CHANNEL) + FILTERTYPESIZE; // �𓀃f�[�^�P�s������̃T�C�Y
	INTE  data_size = (png_info.image_width * CHANNEL); // ��L�t�B���^�^�C�v�����T�C�Y
	INTE  image_y = 0; // Y���W
	INTE  image_x = 0; // X���W
	USCH  filter = 0;  // �t�B���^�^�C�v
	USCH  result = 0;  // ���������̌v�Z����

	// ��������
	// ���A��A����s�N�Z���̒l�ƃt�B���^�^�C�v���l������������
	for (image_y = 0; image_y < png_info.image_height; image_y++) {

		// �|�C���^�����̍s�̐擪�ɐi�߂�
		if (image_y != 0) {
			unzip_ptr += 1;
		}

		// �t�B���^�^�C�v�擾
		filter = *unzip_ptr;
		unzip_ptr += 1;

		for (image_x = 0; image_x < data_size; image_x++) {

			// ���ݒn�A���A��A����̒l���m�ہi��r�p�j
			USCH curr = 0;
			USCH left = 0;
			USCH uppr = 0;
			USCH l_up = 0;

			// ���ݒn
			curr = *unzip_ptr;
			// ��
			if (image_x >= CHANNEL) {
				unzip_ptr -= CHANNEL;
				left = *unzip_ptr;
				unzip_ptr += CHANNEL;
			}
			else {
				left = 0;
			}
			// ��
			if (image_y != 0) {
				unzip_ptr -= line_size;
				uppr = *unzip_ptr;
				unzip_ptr += line_size;
			}
			else {
				uppr = 0;
			}
			// ����
			if (image_x >= CHANNEL && image_y != 0) {
				unzip_ptr -= (line_size + CHANNEL);
				l_up = *unzip_ptr;
				unzip_ptr += (line_size + CHANNEL);
			}
			else {
				l_up = 0;
			}

			//�@�v�Z���ʏ�����
			result = 0;

			// ����
			if (filter == 0x00) {
				result = curr;

			}
			else if (filter == 0x01) {
				result = (USSH)(curr + left) & 0xff;

			}
			else if (filter == 0x02) {
				result = (USSH)(curr + uppr) & 0xff;

			}
			else if (filter == 0x03) {
				result = (USSH)(curr + ((uppr + left) / 2)) & 0xff;

			}
			else if (filter == 0x04) {
				INTE pre = left + uppr - l_up;
				INTE p_l = abs(pre - left);
				INTE p_u = abs(pre - uppr);
				INTE plu = abs(pre - l_up);
				INTE paeth = 0;

				if (p_l <= p_u && p_l <= plu) {
					paeth = left;
				}
				else if (p_u <= p_l && p_u <= plu) {
					paeth = uppr;
				}
				else {
					paeth = l_up;
				}
				result = (USSH)(curr + paeth) & 0xff;

			}
			else {
				printf("Unsupported filter type.\n");
				return 1;
			}

			// ���ʏ�������
			*unzip_ptr = result;
			if (image_x != data_size - 1) {
				unzip_ptr += 1;
			}

		}

	}

	return 0;
}

/*================================================================================
* _delete_scanline_() RGB�f�[�^�ɕt������X�L�������C���f�[�^���폜
================================================================================*/
INTE TO_BMP::_delete_scanline_(USCH* unzip_ptr, USCH* rgb_ptr, INTE data_size, PNGDATA& png_info) {

	// �����|�C���^�̒��g�m�F
	if (unzip_ptr == NULL || rgb_ptr == NULL) { return 1; };

	INTE image_y = 0; // �摜�̍s��
	INTE image_x = 0; // �摜�̗�

	// ���o����
	unzip_ptr += 1; // �t�B���^�^�C�v�̂P����

	for (image_y = 0; image_y < png_info.image_height; image_y++) {
		for (image_x = 0; image_x < png_info.image_width * CHANNEL; image_x++) {
			*rgb_ptr = *unzip_ptr;
			rgb_ptr += 1;
			unzip_ptr += 1;
		}
		unzip_ptr += 1;
	}

	return 0;
}

/*================================================================================
* _premulti_alpha_() �v���}���`�v���C����
================================================================================*/
INTE TO_BMP::_premulti_alpha_(USCH* rgb_ptr, INTE bmp_size) {

	// �����|�C���^�̒��g�m�F
	if (rgb_ptr == NULL) { return 1; };

	INTE index = 0; // �z�񑀍�p

	// �X�g���[�gRGB�̒l�ɃA���t�@�l����Z���邱�Ƃ�
	// �v���}���`�v���C�hRGB�����܂�
	for (index = 0; index < bmp_size; index += CHANNEL) {

		USCH R = rgb_ptr[index + 0];
		USCH G = rgb_ptr[index + 1];
		USCH B = rgb_ptr[index + 2];
		USCH A = rgb_ptr[index + 3];

		rgb_ptr[index + 0] = (USCH)((R * A) / 255);
		rgb_ptr[index + 1] = (USCH)((G * A) / 255);
		rgb_ptr[index + 2] = (USCH)((B * A) / 255);

	}

	return 0;
}

/*================================================================================
* _rgb_to_bgr_() RGB�f�[�^��BGR�ɕϊ�
================================================================================*/
INTE TO_BMP::_rgb_to_bgr_(USCH* rgb_ptr, INTE bmp_size, PNGDATA& png_info) {

	// �����|�C���^�̒��g�m�F
	if (rgb_ptr == NULL) { return 1; };

	INTE index = 0;

	// BGR�ϊ�����
	for (index = 0; index < bmp_size; index += CHANNEL) {
		if (index + 2 >= bmp_size) {
			break;
		}
		USCH tmp = rgb_ptr[index];
		rgb_ptr[index] = rgb_ptr[index + 2];
		rgb_ptr[index + 2] = tmp;
	}

	return 0;
}

/*================================================================================
* _flip_side_up_() BGR�f�[�^���㉺���]
================================================================================*/
INTE TO_BMP::_flip_side_up_(USCH* rgb_ptr, USCH* bmp_ptr, PNGDATA& png_info) {

	// �����|�C���^�̒��g�m�F
	if (rgb_ptr == NULL || bmp_ptr == NULL) { return 1; };

	INTE  data_size = png_info.image_width * CHANNEL;

	// RGB�f�[�^�̍ŏI�s�̐擪�Ɉړ�
	rgb_ptr += (data_size * png_info.image_height) - 1;
	rgb_ptr -= (data_size - 1);

	INTE index_x = 0;
	INTE index_y = 0;

	// RGB�f�[�^�̍ŏI�s����擪�s�ɂ����āAbmp_ptr�̃������ɏ�������
	for (index_y = 0; index_y < png_info.image_height; index_y++) {
		for (index_x = 0; index_x < data_size; index_x++) {
			*bmp_ptr = *rgb_ptr;
			bmp_ptr += 1;
			rgb_ptr += 1;
		}
		// 2�s�O�̐擪�Ɉړ�
		rgb_ptr -= data_size * 2;
	}

	return 0;
}

/*================================================================================
* _save_bmp_() BMP�Ƃ��ďo��
================================================================================*/
INTE TO_BMP::_save_bmp_(USCH* bmp_ptr, PNGDATA& png_info, INTE bmp_size) {

	// �����|�C���^�̒��g�m�F
	if (bmp_ptr == NULL) { return 1; };

	FILE* file_ptr = NULL;

	// �t�@�C���̐���
	fopen_s(&file_ptr, "./output/output.bmp", "wb");
	if (file_ptr == NULL) {
		printf("File open error.\n");
		return 1;
	}

	// �w�b�_�[��������
	struct BMPFILEHEADER bmp_file_header;
	bmp_file_header.bfType          = 0x4d42; // ASCII��"BM"
	bmp_file_header.bfSize          = sizeof(BMPFILEHEADER) + sizeof(BMPINFOHEADER) + bmp_size;
	bmp_file_header.bfReserved1     = 0;
	bmp_file_header.bfReserved2     = 0;
	bmp_file_header.bfOffset        = sizeof(BMPFILEHEADER) + sizeof(BMPINFOHEADER);

	struct BMPINFOHEADER bmp_info_header;
	bmp_info_header.biSize	        = sizeof(BMPINFOHEADER);
	bmp_info_header.biWidth		    = png_info.image_width;
	bmp_info_header.biHeight		= png_info.image_height;
	bmp_info_header.biPlanes		= 1;
	bmp_info_header.biBitCount	    = 0x20; // RGBA��32bit
	bmp_info_header.biCompression   = 0;
	bmp_info_header.biSizeImage     = bmp_size;
	bmp_info_header.biXPelsPerMeter = 0;
	bmp_info_header.biYPelsPerMeter = 0;
	bmp_info_header.biClrUsed       = 0;
	bmp_info_header.biClrImportant  = 0;

	// �w�b�_�[��������
	// write_ret��fwrite�������������ǂ����̔���
	INTE write_ret = 0;

	// FILEHEADER�̏�������
	write_ret = fwrite(&bmp_file_header, sizeof(BMPFILEHEADER), 1, file_ptr);
	if (write_ret != 1) { printf("File write error.\n");  return 1; }

	// INFOHEADER�̏�������
	write_ret = fwrite(&bmp_info_header, sizeof(BMPINFOHEADER), 1, file_ptr);
	if (write_ret != 1) { printf("File write error.\n");  return 1; }

	// RGB�f�[�^��������
	write_ret = fwrite(bmp_ptr, bmp_size, 1, file_ptr);
	if (write_ret != 1) { printf("File write error.\n");  return 1; }

	// �I��
	fclose(file_ptr);

	printf("Process successfully.\n");
	printf("Check the output file.\n");

	return 0;
}

