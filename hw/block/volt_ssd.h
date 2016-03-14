#ifndef VOLT_SSD_H
#define VOLT_SSD_H

#include<stdint.h>
#include "include/qemu/timer.h"

#define LNVM_VOLT_MEM_ERROR 0
#define LNVM_VOLT_MEM_OK 1
#define LNVM_VOLT_SECOND 1000000 /* from u-seconds */

/* should be user-defined */
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
    int64_t     allocated_memory;
} LnvmVoltStatus;

typedef struct LnvmVoltPage {
    uint8_t     state; /* 0-free, 1-alive, 2-invalid */
} LnvmVoltPage;

typedef struct LnvmVoltBlock {
    uint16_t        life; /* available writes before die */
    LnvmVoltPage    *next_pg;
    LnvmVoltPage    *pages;
    uint8_t         *data;
} LnvmVoltBlock;

typedef struct LnvmVoltLun {
    LnvmVoltBlock   *blk_offset;
} LnvmVoltLun;

typedef struct LnvmVoltCtrl {
    struct NvmeCtrl *ctrl;
    struct NvmeNamespace   *ns;
    LnvmVoltParams  params;
    LnvmVoltStatus  status;
    LnvmVoltBlock   *blocks;
    LnvmVoltLun     *luns;
    QEMUTimer       *mainTimer;
} LnvmVoltCtrl;

void nvme_volt_init(struct NvmeCtrl *n);
void nvme_volt_main(void *ctrl); /* main thread */

#endif /* VOLT_SSD_H */ 