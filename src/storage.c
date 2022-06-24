//
// Created by aron on 21.06.22.
//
#include <stdio.h>
#include <string.h>

#include "storage.h"
#include "f_util.h"
#include "ff.h"
#include "hw_config.h"
#include "serial.h"



#define RECORDS_DIR "MIC_REC"
#define FILE_EXTENSION ".raw"
#define FILETYPE_SEARCH_PATTERN "*.raw"
#define FILENAME_MAX_LEN 15
#define FILENAME_NUMBER_LEN 4
#define MAX_FILE_LEN 76800000

uint nextFileNumber = 0;
char nextFilename[FILENAME_MAX_LEN] = {0};
char curFilepath[FILENAME_MAX];
FIL curFile;
size_t curFileSize;
static uint bufElems = 0;
static uint32_t outBuf[STORAGE_OUTPUT_BUF_LEN];


uint get_file_number(const char * name){
	uint number = 0;
	for(int i = 0; i < FILENAME_MAX_LEN; i++){
		if(name[i] < '0' || name[i] > '9' ){
			break;
		}
		number = number * 10 + (uint) name[i] - (uint) '0';
	}
	return number;
}

void create_next_filename(uint num){
	memset(nextFilename, 0, FILENAME_MAX_LEN);
	for (int i = FILENAME_NUMBER_LEN - 1; i >= 0; --i){
		nextFilename[i] = num % 10 + (int)'0';
		num /= 10;
	}
	strcat(nextFilename, FILE_EXTENSION);
}

int set_next_filename(){
	FILINFO file;
	DIR dir;
	FRESULT fr;
	uint num;

	memset(&file, 0, sizeof(file));
	memset(&dir, 0, sizeof(dir));

	fr = f_findfirst(&dir, &file, RECORDS_DIR, FILETYPE_SEARCH_PATTERN);
	if(FR_OK != fr){
		UART_LOG("f_findfirst error\n");
		return 0;
	}
	while (fr == FR_OK && file.fname[0]) {
		num = get_file_number(file.fname);
		if(num >= nextFileNumber){
			nextFileNumber = num + 1;
		}
		fr = f_findnext(&dir, &file);
	}
	create_next_filename(nextFileNumber);
	return 1;
}

int storage_sdcard_init(){
	sd_card_t *pSD = sd_get_by_num(0);
	if( pSD == NULL){
		return 0;
	}
	FATFS *p_fs = &pSD->fatfs;
	if (!p_fs) {
		UART_LOG("Unknown logical drive\n");
		return 0;
	}
	FRESULT fr = f_mount(p_fs, pSD->pcName, 1);
	if (FR_OK == fr) {
		pSD->mounted = true;

		fr = f_mkdir(RECORDS_DIR);
		if(FR_OK != fr && FR_EXIST != fr){
			UART_LOG("f_mkdir error\n");
			return 0;
		}

		return set_next_filename();
	}

	UART_LOG("f_mount error\n");

	return 0;
}

int storage_create_file(){
	FRESULT fr;
	memset(curFilepath, 0, FILENAME_MAX);
	create_next_filename(nextFileNumber);
	strcat(curFilepath, RECORDS_DIR);
	strcat(curFilepath, "/");
	strncat(curFilepath, nextFilename, FILENAME_MAX_LEN);

	fr = f_close(&curFile);
	if(FR_OK != fr && FR_NO_FILE != fr && FR_INVALID_OBJECT != fr){
		UART_LOG("f_close error\n");
		return 0;
	}

	fr = f_open(&curFile, curFilepath, FA_CREATE_NEW | FA_WRITE );
	if(FR_OK != fr){
		UART_LOG("f_open error\n");
		return 0;
	}

	fr = f_close(&curFile);
	if(FR_OK != fr && FR_NO_FILE != fr && FR_INVALID_OBJECT != fr){
		UART_LOG("f_close error\n");
		return 0;
	}

	nextFileNumber++;
	curFileSize = 0;
	return 1;
}

int storage_setup_output_chunk(uint32_t * chunk, uint len){
	if(STORAGE_OUTPUT_BUF_LEN - bufElems >= len){
		for(int i = 0; i < len; ++i){
			outBuf[bufElems] = chunk[i];
			++bufElems;
		}
		return 1;
	}
	return 0;

}

int storage_write_data_chunk(){
	FRESULT fr;
	FIL file;
	uint writen;
	uint len;

	len = bufElems * sizeof(uint32_t);
	fr = f_open(&file, curFilepath, FA_OPEN_APPEND | FA_WRITE);
	if(FR_OK != fr){
		UART_LOG("f_open data chunk error");
		return 0;
	}
	fr = f_write(&file,outBuf, len, &writen);
	if(FR_OK != fr){
		UART_LOG("f_write data chunk error");
		return 0;
	}
	curFileSize += writen;
	bufElems = 0;
	fr = f_close(&file);
	if(FR_OK != fr){
		UART_LOG("f_close error");
		return 0;
	}
	if(MAX_FILE_LEN - curFileSize < len){
		create_next_filename(nextFileNumber);
		if(storage_create_file() == 0){
			return 0;
		}
	}
	return 1;
}

void storage_close_file(){
	FRESULT fr = f_close(&curFile);
	if(FR_OK != fr){
		UART_LOG("f_close error\n");
	}
}