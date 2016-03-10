#ifndef VOLT_SSD_H
#define VOLT_SSD_H

#include<stdint.h>

#define LNVM_VOLT_BLK_LIFE 5000

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
    uint8_t state; /* 0-free, 1-alive, 2-invalid */
} LnvmVoltPage;

typedef struct LnvmVoltBlock {
    uint16_t life; /* available writes before die */
    LnvmVoltPage *pages;
    uint8_t *data;
} LnvmVoltBlock;

typedef struct LnvmVoltLun {
    LnvmVoltBlock *blk_offset;
} LnvmVoltLun;

typedef struct LnvmVoltCtrl {
    LnvmVoltParams params;
    LnvmVoltStatus status;
    LnvmVoltBlock * blocks;
    LnvmVoltLun * luns;
} LnvmVoltCtrl;

/* TODO: after the work is done, change void to NvmeCtrl */
void nvme_volt_init(void *ctrl);
void * nvme_volt_main(void *ctrl); /* main thread */

#endif /* VOLT_SSD_H */ 