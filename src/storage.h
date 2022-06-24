//
// Created by aron on 21.06.22.
//

#ifndef RECORDER_STORAGE_H
#define RECORDER_STORAGE_H

#define STORAGE_OUTPUT_BUF_LEN 32768

int storage_sdcard_init();
int storage_create_file();
int storage_setup_output_chunk(uint32_t * chunk, uint len);
int storage_write_data_chunk();
void storage_close_file();

#endif //RECORDER_STORAGE_H
