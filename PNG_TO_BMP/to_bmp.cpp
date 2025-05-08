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
#define MAX_PATH		 260       // ファイル名最大文字数
#define PATH_START		 6         // "input/"を考慮した配列開始位置
#define MAX_FILE_SIZE	 10000000  // 入力画像の最大サイズ(10MB)
#define ID_SIZE			 8         // PNG識別子サイズ
#define TO_WIDTH		 16        // pngの横幅データまでのバイト数
#define TO_HEIGHT		 20        // pngの高さデータまでのバイト数
#define TO_BITDEPTH		 24        // 色深度までのバイト数
#define TO_COLORTYPE     25        // カラータイプまでのバイト数
#define TO_PRESSMETHOD   26        // 圧縮形式までのバイト数
#define TO_FILTERMETHOD  27        // フィルタ方式までのバイト数
#define TO_INTERMETHOD   28        // インタレース方式までのバイト数
#define MAX_PIXEL        5000      // 画像幅、高さの最大ピクセル数
#define BIT_DEPTH        0x08      // 許可するビット深度
#define COLOR_TYPE       0x06      // 許可するカラータイプ
#define PRESS_METHOD     0x00      // 許可する圧縮方式
#define FILTERTYPE       0x00      // 許可するフィルタ方式
#define INTERMETHOD      0x00      // 許可するインタレース方式
#define CHUNK_BITE       4         // IDAT, IENDを探索する際に確認するバイト数
#define TO_CHUNKSIZE     8         // IDATチャンクの識別子からサイズを取得する際に戻るバイト数
#define CHANNEL          4         // RGBAのチャネル数
#define FILTERTYPESIZE   1         // 解凍後データの行ごとの先頭に付与されるフィルタタイプのバイト数

/*================================================================================
* _exe_() クラス内関数の実行
================================================================================*/
INTE TO_BMP::_exe_() {

	FILE* input_file_ptr = NULL;
	CHAR  input_file_name[MAX_PATH] = "input/"; // 入力ファイル名配列
	USCH  func_ret = 0;

	// 画像情報
	struct PNGDATA png_info;
	memset(&png_info, 0, sizeof(struct PNGDATA));

	// =====================================================================
	// 加工対象ファイル名の取得
	// 存在確認後、フォーマットや画像情報の検証
	// =====================================================================
	while (1) {

		// ファイル名入力
		printf("Enter target file name. (e.g. image.png):");
		scanf_s("%s", &input_file_name[PATH_START], MAX_PATH - PATH_START);

		// 存在確認
		fopen_s(&input_file_ptr, input_file_name, "rb");
		if (input_file_ptr == NULL) {
			printf("File not found, Please re-enter.\n");
			continue;
		}

		// ファイルフォーマットの検証
		func_ret = _check_format_(input_file_ptr);
		if (func_ret != 0) { fclose(input_file_ptr); return 1; }

		// 画像情報取得、検証
		_get_info_(input_file_ptr, png_info);
		func_ret = _check_info_(png_info);
		if (func_ret != 0) { fclose(input_file_ptr); return 1; }

		// 検証処理終了
		printf("Start processing...\n");
		break;

	}

	// pngの画像データ用メモリを確保、PNG画像データ取得
	USCH* png_first_ptr = (USCH*)malloc(png_info.image_size);
	if (png_first_ptr == NULL) {
		printf("Memory Error 1001\n");
		fclose(input_file_ptr);
		return 1;
	}

	// pngデータを確保
	INTE read_ret = 0;
	read_ret = fread(png_first_ptr, png_info.image_size, 1, input_file_ptr);

	// 画像ファイル開放
	fclose(input_file_ptr);

	// =====================================================================
	// IDATチャンクを取得（メモリのポインタはリストに格納）
	// =====================================================================
	// 各チャンクデータが格納されたメモリの先頭アドレスリスト
	list<USCH*>           _FirstIdatPtrList; // IDAT先頭ポインタリスト
	list<USCH*>::iterator _CPIte;            // 上記イテレータ
	// 各チャンクのサイズのリスト
	list<USIN>            _ChunkSizeList;    // IDATサイズリスト
	list<USIN>::iterator  _CSIte;            // 上記イテレータ

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
	// IDATチャンクの連結する
	// 連結後サイズの計算し、連結したIDATチャンクを格納するメモリを確保
	// =====================================================================
	USIN idat_size = 0;
	for (_CSIte  = _ChunkSizeList.begin(); _CSIte != _ChunkSizeList.end(); _CSIte++) {
		idat_size += *_CSIte;
	}

	// メモリ確保
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

	// 連結実行
	_link_idat_(idat_first_ptr, _FirstIdatPtrList, _CPIte, _ChunkSizeList, _CSIte);
	_FirstIdatPtrList.clear();
	_ChunkSizeList.clear();

	// =====================================================================
	// 連結したIDATデータを解凍する
	// 解凍後サイズを計算し、解凍後データを格納するメモリを確保
	// =====================================================================
	USIN unzip_size = ((png_info.image_width * CHANNEL) + FILTERTYPESIZE) * png_info.image_height;

	// メモリ確保
	USCH* unzip_first_ptr = (USCH*)malloc(unzip_size);
	if (unzip_first_ptr == NULL) {
		printf("Memory Error 1004\n");
		free(idat_first_ptr);
		return 1;
	}

	// 解凍実行
	func_ret = _unzip_idat_(idat_first_ptr, idat_size, unzip_first_ptr, unzip_size);
	free(idat_first_ptr);
	if (func_ret != 0) {
		free(unzip_first_ptr);
		return 1; 
	}

	// =====================================================================
	// 解凍して得たスキャンラインデータをRGBデータに復元する
	// =====================================================================
	func_ret = _scanline_decode_(unzip_first_ptr, png_info);
	if (func_ret != 0) {
		free(unzip_first_ptr);
		return 1;
	}

	// =====================================================================
	// 現時点の解凍データにはRGBデータのほかに
	// スキャンラインが含まれているため、それを除去する
	// =====================================================================
	INTE data_size = (png_info.image_width * CHANNEL); // １行当たりのフィルタタイプ抜きデータサイズ
	INTE rgb_size = data_size * png_info.image_height;

	// フィルタタイプを除去したRGBデータを格納するメモリ
	USCH* rgb_first_ptr = (USCH*)malloc(rgb_size);
	if (rgb_first_ptr == NULL) {
		printf("Memory Error 1005\n");
		free(unzip_first_ptr);
		return 1;
	}

	// 除去実行
	func_ret = _delete_scanline_(unzip_first_ptr, rgb_first_ptr, data_size, png_info);
	free(unzip_first_ptr);
	if (func_ret != 0) {
		free(rgb_first_ptr);
		return 1;
	}

	// =====================================================================
	// アルファ値がFF(透明)でもRGBデータを持っているために
	// BMPに出力する際にその色が出てきてしまうことがある
	// そのため、アルファ値を補正する処理を行う
	// =====================================================================
	INTE  bmp_size = png_info.image_width * png_info.image_height * CHANNEL;
	_premulti_alpha_(rgb_first_ptr, bmp_size);

	// =====================================================================
	// BMPにデータを格納するするためにRGBデータをBGRデータに変換する
	// =====================================================================
	_rgb_to_bgr_(rgb_first_ptr, bmp_size, png_info);

	// =====================================================================
	// BMPデータとしてそのまま書き込むために現在のBGRデータを上下反転する
	// bmpデータを格納するメモリを確保
	// =====================================================================
	USCH* bmp_first_ptr = (USCH*)malloc(bmp_size);
	if (bmp_first_ptr == NULL) {
		printf("Memory Error 1006\n");
		free(rgb_first_ptr);
		return 1;
	}
	// 上下反転処理実行
	_flip_side_up_(rgb_first_ptr, bmp_first_ptr, png_info);

	// メモリ開放
	free(rgb_first_ptr);

	// =====================================================================
	// 保存処理
	// =====================================================================
	func_ret = _save_bmp_(bmp_first_ptr, png_info, bmp_size);
	if (func_ret != 0) { 
		free(bmp_first_ptr);
		return 1; 
	}
	free(bmp_first_ptr);

	// exe終了
	return 0;
}

/*================================================================================
* _check_format_() 入力画像がpng形式かを判定
================================================================================*/
INTE TO_BMP::_check_format_(FILE* input_file_ptr) {

	// 引数ポインタの中身確認
	if (input_file_ptr == NULL) { return 1; }

	// PNG識別子を格納した配列と取得した先頭８バイトを比較
	USCH png_id [ID_SIZE] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
	USCH png_buf[ID_SIZE];
	USCH flg   = 0;
	INTE index = 0;

	// 配列初期化 
	for (index = 0; index < ID_SIZE; index++) {
		png_buf[index] = 0;
	}

	// 先頭８バイト（PNG識別子）読み取り
	INTE read_ret = 0;

	fseek(input_file_ptr, 0, SEEK_SET);
	fread(&png_buf[0], ID_SIZE, 1, input_file_ptr);
	fseek(input_file_ptr, 0, SEEK_SET);

	// 配列の中身を比較
	for (index = 0; index < 8; index++) {
		if (png_id[index] != png_buf[index]) {
			flg++;
		}
	}

	// 判定
	if (flg == 0) {
		return 0;
	}
	else {
		printf("Please enter a png image.\n");
		return 1;
	}

}

/*================================================================================
* _get_info_() 入力画像の情報を取得
================================================================================*/
INTE TO_BMP::_get_info_(FILE* input_file_ptr, PNGDATA& png_info) {

	// 引数ポインタの中身確認
	if (input_file_ptr == NULL) { return 1; };

	// ファイルサイズ取得
	fseek(input_file_ptr, 0, SEEK_END);
	png_info.image_size = ftell(input_file_ptr);
	fseek(input_file_ptr, 0, SEEK_SET);

	// IHDRチャンク内の画像情報を取得
	INTE tmp4 = 0;     // ４バイトデータ取得用
	USCH tmp1 = 0;     // １バイトデータ取得用

	// 画像幅
	fseek(input_file_ptr, TO_WIDTH, SEEK_SET);
	fread(&tmp4, sizeof(tmp4), 1, input_file_ptr);
	png_info.image_width = endian_conv(tmp4);

	// 画像高
	fseek(input_file_ptr, TO_HEIGHT, SEEK_SET);
	fread(&tmp4, sizeof(tmp4), 1, input_file_ptr);
	png_info.image_height = endian_conv(tmp4);

	// ビット深度
	fseek(input_file_ptr, TO_BITDEPTH, SEEK_SET);
	fread(&png_info.bit_depth, sizeof(tmp1), 1, input_file_ptr);

	// カラータイプ
	fseek(input_file_ptr, TO_COLORTYPE, SEEK_SET);
	fread(&png_info.color_type, sizeof(tmp1), 1, input_file_ptr);

	// 圧縮方式
	fseek(input_file_ptr, TO_PRESSMETHOD, SEEK_SET);
	fread(&png_info.press, sizeof(tmp1), 1, input_file_ptr);

	// フィルタ方式
	fseek(input_file_ptr, TO_FILTERMETHOD, SEEK_SET);
	fread(&png_info.filter, sizeof(tmp1), 1, input_file_ptr);

	// インタレース方式
	fseek(input_file_ptr, TO_INTERMETHOD, SEEK_SET);
	fread(&png_info.display, sizeof(tmp1), 1, input_file_ptr);

	return 0;

}

/*================================================================================
* _check_info_() 入力画像の情報を検証
================================================================================*/
INTE TO_BMP::_check_info_(PNGDATA& png_info) {

	// 画像サイズの検証（最大10MB）
	if (png_info.image_size > MAX_FILE_SIZE) {
		printf("Image size is too large.(MAX 10MB)\n");
		return 1;
	}
	// 画像幅、画像高さの最大pxは5000x5000
	if (png_info.image_width > MAX_PIXEL || png_info.image_height > MAX_PIXEL) {
		printf("Image size is too large.(MAX 5000x5000px)\n");
		return 1;
	}
	// ビット深度は0x08のもの限定
	if (png_info.bit_depth != BIT_DEPTH) {
		printf("The bit depth of this image is not supported.(only 0x08)\n");
		return 1;
	}
	// カラータイプは0x06のもの限定
	if (png_info.color_type != COLOR_TYPE) {
		printf("The color type of this image is not supported.(only 0x06)\n");
		return 1;
	}
	// 圧縮方式は0x00のもの限定
	if (png_info.press != PRESS_METHOD) {
		printf("The compression method of this image is not supported.(only 0x00)\n");
		return 1;
	}
	// フィルタ方式は0x00のもの限定
	if (png_info.filter != FILTERTYPE) {
		printf("%02x", png_info.filter);
		printf("The filter method of this image is not supported.(only 0x00)\n");
		return 1;
	}
	// インタレース方式は0x00のもの限定
	if (png_info.display != INTERMETHOD) {
		printf("The interlace method of this image is not supported.(only 0x00)\n");
		return 1;
	}

	return 0;
}

/*================================================================================
* _get_idat_() IDATチャンクを取得する
================================================================================*/
INTE TO_BMP::_get_idat_(USCH* png_ptr, list<USCH*>& ptr_list, list<USIN>& size_list) {

	// 引数ポインタの中身確認
	if (png_ptr == NULL) { return 1; };

	USCH iend_id[4] = { 0x49, 0x45, 0x4e, 0x44 }; // IENDチャンクの識別子
	USCH idat_id[4] = { 0x49, 0x44, 0x41, 0x54 }; // IDATチャンクの識別子
	USCH search [4];
	USIN index = 0;

	// IENDチャンクが見つかるまでIDATチャンクを探索する
	// IDATチャンク発見後、データを取得し、サイズと先頭ポインタをクラス変数のリストに格納
	while (1) {

		// search初期化 
		for (index = 0; index < CHUNK_BITE; index++) {
			search[index] = 0;
		}

		// 4バイト取得
		for (index = 0; index < CHUNK_BITE; index++) {
			search[index] = *png_ptr;
			png_ptr++;
		}

		// IENDチャンクで終了
		if (search[0] == iend_id[0] && search[1] == iend_id[1] &&
			search[2] == iend_id[2] && search[3] == iend_id[3]) {
			break;
		}

		// IDATチャンク発見後、IDATのデータを取得
		// 取得したデータサイズとデータの先頭ポインタをリストに格納
		if (search[0] == idat_id[0] && search[1] == idat_id[1] &&
			search[2] == idat_id[2] && search[3] == idat_id[3]) {

			// チャンクサイズを取得
			png_ptr -= TO_CHUNKSIZE;
			USIN idat_size = *((USIN*)png_ptr);
			idat_size = endian_conv(idat_size);

			size_list.push_back(idat_size);

			// サイズを取得したので、IDATの先頭へ戻る
			png_ptr += TO_CHUNKSIZE;

			// IDATサイズ分のメモリ確保
			USCH* idat_first_ptr = (USCH*)malloc(idat_size);
			if (idat_first_ptr == NULL) {
				printf("Memory Error 1002\n");
				return 1;
			}
			USCH* idat_ptr = idat_first_ptr; // 操作用

			// IDATデータ確保
			memcpy(idat_ptr, png_ptr, idat_size);
			png_ptr += idat_size + CHUNK_BITE;

			// データ取得後、リストに先頭アドレスを格納し、データ取得が終了する
			ptr_list.push_back(idat_first_ptr);

		}
		else {
			// １つずつスタートをずらしながら探索を行う
			// 4バイト探索し、3バイト戻る...
			png_ptr -= 3;
		}

	}

	return 0;
}

/*================================================================================
* _link_idat_() IDATチャンクを連結
================================================================================*/
INTE TO_BMP::_link_idat_(USCH* idat_ptr, list<USCH*>& ptr_list, list<USCH*>::iterator ptr_ite, list<USIN>& size_list, list<USIN>::iterator size_ite) {

	// 引数ポインタの中身確認
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
* _unzip_idat_() IDATデータを解凍
================================================================================*/
INTE TO_BMP::_unzip_idat_(USCH* idat_ptr, INTE idat_size, USCH* unzip_ptr, INTE unzip_size) {

	// 引数ポインタの中身確認
	if (idat_ptr == NULL || unzip_ptr == NULL) { return 1; };

	// z_stream初期化
	z_stream stream;
	stream.zalloc   = Z_NULL;
	stream.zfree    = Z_NULL;
	stream.opaque   = Z_NULL;
	stream.next_in  = idat_ptr;  // IDATデータ先頭
	stream.avail_in = idat_size; // IDATデータサイズ

	// inflate初期化
	INTE ret = inflateInit(&stream);
	if (ret != Z_OK) {
		printf("init error: %d\n", ret);
		return 1;
	}

	// 構造体に解凍後データ詳細を設定
	stream.next_out  = unzip_ptr;   // 解凍後データ先頭
	stream.avail_out = unzip_size;  // 解凍後データサイズ

	// 解凍
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
* _scanline_decode_() 解凍したIDATデータをRGBに復元
================================================================================*/
INTE TO_BMP::_scanline_decode_(USCH* unzip_ptr, PNGDATA& png_info) {

	// 引数ポインタの中身確認
	if (unzip_ptr == NULL) { return 1; };

	// 変数宣言
	INTE  line_size = (png_info.image_width * CHANNEL) + FILTERTYPESIZE; // 解凍データ１行当たりのサイズ
	INTE  data_size = (png_info.image_width * CHANNEL); // 上記フィルタタイプ抜きサイズ
	INTE  image_y = 0; // Y座標
	INTE  image_x = 0; // X座標
	USCH  filter = 0;  // フィルタタイプ
	USCH  result = 0;  // 復号処理の計算結果

	// 復元処理
	// 左、上、左上ピクセルの値とフィルタタイプを考慮し復元する
	for (image_y = 0; image_y < png_info.image_height; image_y++) {

		// ポインタを次の行の先頭に進める
		if (image_y != 0) {
			unzip_ptr += 1;
		}

		// フィルタタイプ取得
		filter = *unzip_ptr;
		unzip_ptr += 1;

		for (image_x = 0; image_x < data_size; image_x++) {

			// 現在地、左、上、左上の値を確保（比較用）
			USCH curr = 0;
			USCH left = 0;
			USCH uppr = 0;
			USCH l_up = 0;

			// 現在地
			curr = *unzip_ptr;
			// 左
			if (image_x >= CHANNEL) {
				unzip_ptr -= CHANNEL;
				left = *unzip_ptr;
				unzip_ptr += CHANNEL;
			}
			else {
				left = 0;
			}
			// 上
			if (image_y != 0) {
				unzip_ptr -= line_size;
				uppr = *unzip_ptr;
				unzip_ptr += line_size;
			}
			else {
				uppr = 0;
			}
			// 左上
			if (image_x >= CHANNEL && image_y != 0) {
				unzip_ptr -= (line_size + CHANNEL);
				l_up = *unzip_ptr;
				unzip_ptr += (line_size + CHANNEL);
			}
			else {
				l_up = 0;
			}

			//　計算結果初期化
			result = 0;

			// 復号
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

			// 結果書き込み
			*unzip_ptr = result;
			if (image_x != data_size - 1) {
				unzip_ptr += 1;
			}

		}

	}

	return 0;
}

/*================================================================================
* _delete_scanline_() RGBデータに付随するスキャンラインデータを削除
================================================================================*/
INTE TO_BMP::_delete_scanline_(USCH* unzip_ptr, USCH* rgb_ptr, INTE data_size, PNGDATA& png_info) {

	// 引数ポインタの中身確認
	if (unzip_ptr == NULL || rgb_ptr == NULL) { return 1; };

	INTE image_y = 0; // 画像の行数
	INTE image_x = 0; // 画像の列数

	// 抽出処理
	unzip_ptr += 1; // フィルタタイプの１つ次へ

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
* _premulti_alpha_() プリマルチプライ処理
================================================================================*/
INTE TO_BMP::_premulti_alpha_(USCH* rgb_ptr, INTE bmp_size) {

	// 引数ポインタの中身確認
	if (rgb_ptr == NULL) { return 1; };

	INTE index = 0; // 配列操作用

	// ストレートRGBの値にアルファ値を乗算することで
	// プリマルチプライドRGBが求まる
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
* _rgb_to_bgr_() RGBデータをBGRに変換
================================================================================*/
INTE TO_BMP::_rgb_to_bgr_(USCH* rgb_ptr, INTE bmp_size, PNGDATA& png_info) {

	// 引数ポインタの中身確認
	if (rgb_ptr == NULL) { return 1; };

	INTE index = 0;

	// BGR変換処理
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
* _flip_side_up_() BGRデータを上下反転
================================================================================*/
INTE TO_BMP::_flip_side_up_(USCH* rgb_ptr, USCH* bmp_ptr, PNGDATA& png_info) {

	// 引数ポインタの中身確認
	if (rgb_ptr == NULL || bmp_ptr == NULL) { return 1; };

	INTE  data_size = png_info.image_width * CHANNEL;

	// RGBデータの最終行の先頭に移動
	rgb_ptr += (data_size * png_info.image_height) - 1;
	rgb_ptr -= (data_size - 1);

	INTE index_x = 0;
	INTE index_y = 0;

	// RGBデータの最終行から先頭行にかけて、bmp_ptrのメモリに書き込む
	for (index_y = 0; index_y < png_info.image_height; index_y++) {
		for (index_x = 0; index_x < data_size; index_x++) {
			*bmp_ptr = *rgb_ptr;
			bmp_ptr += 1;
			rgb_ptr += 1;
		}
		// 2行前の先頭に移動
		rgb_ptr -= data_size * 2;
	}

	return 0;
}

/*================================================================================
* _save_bmp_() BMPとして出力
================================================================================*/
INTE TO_BMP::_save_bmp_(USCH* bmp_ptr, PNGDATA& png_info, INTE bmp_size) {

	// 引数ポインタの中身確認
	if (bmp_ptr == NULL) { return 1; };

	FILE* file_ptr = NULL;

	// ファイルの生成
	fopen_s(&file_ptr, "./output/output.bmp", "wb");
	if (file_ptr == NULL) {
		printf("File open error.\n");
		return 1;
	}

	// ヘッダー書き込み
	struct BMPFILEHEADER bmp_file_header;
	bmp_file_header.bfType          = 0x4d42; // ASCIIで"BM"
	bmp_file_header.bfSize          = sizeof(BMPFILEHEADER) + sizeof(BMPINFOHEADER) + bmp_size;
	bmp_file_header.bfReserved1     = 0;
	bmp_file_header.bfReserved2     = 0;
	bmp_file_header.bfOffset        = sizeof(BMPFILEHEADER) + sizeof(BMPINFOHEADER);

	struct BMPINFOHEADER bmp_info_header;
	bmp_info_header.biSize	        = sizeof(BMPINFOHEADER);
	bmp_info_header.biWidth		    = png_info.image_width;
	bmp_info_header.biHeight		= png_info.image_height;
	bmp_info_header.biPlanes		= 1;
	bmp_info_header.biBitCount	    = 0x20; // RGBAの32bit
	bmp_info_header.biCompression   = 0;
	bmp_info_header.biSizeImage     = bmp_size;
	bmp_info_header.biXPelsPerMeter = 0;
	bmp_info_header.biYPelsPerMeter = 0;
	bmp_info_header.biClrUsed       = 0;
	bmp_info_header.biClrImportant  = 0;

	// ヘッダー書き込み
	// write_retはfwriteが成功したかどうかの判定
	INTE write_ret = 0;

	// FILEHEADERの書き込み
	write_ret = fwrite(&bmp_file_header, sizeof(BMPFILEHEADER), 1, file_ptr);
	if (write_ret != 1) { printf("File write error.\n");  return 1; }

	// INFOHEADERの書き込み
	write_ret = fwrite(&bmp_info_header, sizeof(BMPINFOHEADER), 1, file_ptr);
	if (write_ret != 1) { printf("File write error.\n");  return 1; }

	// RGBデータ書き込み
	write_ret = fwrite(bmp_ptr, bmp_size, 1, file_ptr);
	if (write_ret != 1) { printf("File write error.\n");  return 1; }

	// 終了
	fclose(file_ptr);

	printf("Process successfully.\n");
	printf("Check the output file.\n");

	return 0;
}

