/*================================================================================
* include, header
================================================================================*/
#include<stdio.h>
#include<stdlib.h>
#include"define_type.h"
#include"to_bmp.h"

/*================================================================================
* prosess
================================================================================*/
INTE main(void) {

	INTE ret = 0;

	class TO_BMP TO_BMP_C;
	ret = TO_BMP_C._exe_();

	if (ret != 0) {
		printf("The process did not complete successfully.\n");
	}

	system("pause");
	return 0;

}