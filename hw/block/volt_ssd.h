#ifndef VOLT_SSD_H
#define VOLT_SSD_H

#include<stdint.h>

typedef struct LnvmVoltParams {
    uint8_t     num_ch;
    uint8_t     num_lun;
    uint16_t    num_blk;
    uint16_t    num_pg;
    size_t      pg_size;
} LnvmVoltParams;

typedef struct LnvmVoltStatus {
    uint8_t     ready; /* 0-busy, 1-ready to use */
    uint8_t     active; /* 0-disabled, 1-activated */
} LnvmVoltStatus;

typedef struct LnvmVoltPage {
    void *data;
    uint8_t state; /* 0-free, 1-alive, 2-invalid */
    void *block;
} LnvmVoltPage;

typedef struct LnvmVoltBlock {
    LnvmVoltPage *pages;
    uint16_t life; /* available writes before die */
    void *lun;
} LnvmVoltBlock;

typedef struct LnvmVoltLun {
    LnvmVoltBlock *blocks;
    void *channel;
} LnvmVoltLun;

typedef struct LnvmVoltChannel {
    LnvmVoltLun *luns;
} LnvmVoltCh;

typedef struct LnvmVoltCtrl {
    LnvmVoltParams params;
    LnvmVoltStatus status;
    LnvmVoltBlock * blocks;
} LnvmVoltCtrl;

/* TODO: after the work is done, change void to NvmeCtrl */
void nvme_volt_init(void *ctrl);
void * nvme_volt_main(void *ctrl); /* main thread */

#endif /* VOLT_SSD_H */ 
