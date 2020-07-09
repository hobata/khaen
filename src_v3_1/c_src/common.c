// common.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include "common.h"

int wiringpi_setup_flag = 0;

///////////////////////////////////////////
int r_strcmp(const void *a, const void *b )
{	//reverse
	return -1 * strcmp((const char*)*(char**)a, (const char*)*(char**)b);	
}
void del_file(char *path, int limit)
{
    char *c_str;
    char **c_ptr, **c_ptr_tmp;
    char *pName;
    struct dirent *ent; 
    int cnt = 0, cnt2;
    char tmp_str[STR_LEN];


    DIR *pDir = opendir(path);
    if (pDir == NULL) return;
    //count file num
    while ((ent = readdir(pDir)) != NULL) {
        if (ent->d_type & DT_REG ){
		cnt++;
	}
    }
#if 0
    printf("cnt:%d\n", cnt);
#endif
    if ( cnt > MAX_FILE_NUM){
	cnt = MAX_FILE_NUM;
    }else if ( cnt <= limit){
        closedir(pDir);
	return;
    }
    //file name list
    pName = c_str = calloc(cnt, STR_LEN);
    //file name pointer list
    c_ptr_tmp = c_ptr = calloc(cnt, sizeof(char*));

    cnt2 = cnt;
    pName = c_str;
    rewinddir(pDir);
    //get file name
    while ((ent = readdir(pDir)) != NULL) {
        if (ent->d_type & DT_REG ){
		sprintf(pName, "%s", ent->d_name);
		//make file name pointer list
		*c_ptr_tmp = pName;
		c_ptr_tmp++;
		pName += STR_LEN;
		cnt2--;
		if (cnt2 <= 0) break;
	}
    }
#if 0
    cnt2 = cnt;
    for (pName = c_str; cnt2 > 0; cnt2--, pName += STR_LEN){
	printf("%s**\n", pName);
    }
    printf("\n");
#endif
    closedir(pDir);

    qsort(c_ptr, cnt, sizeof(c_ptr), r_strcmp);
#if 0
    c_ptr_tmp = c_ptr;
    pName = *c_ptr_tmp;
    for (cnt2 = cnt; cnt2 > 0; cnt2--, pName = *(++c_ptr_tmp)){
	printf("%s**\n", pName);
    }
#endif
    c_ptr_tmp = c_ptr + limit;
    pName = *c_ptr_tmp;
    for (cnt2 = limit; cnt2 < cnt; cnt2++, pName = *(++c_ptr_tmp)){
	sprintf(tmp_str, "%s/%s", path, pName);
#if 1
	remove(tmp_str);
#else
	printf("del:%s**\n", pName);
#endif
    }
    //printf("free()1\n");
    free(c_str);
    //printf("free()2\n");
    free(c_ptr);
    //printf("free()3\n");
}
