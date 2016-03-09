#ifndef VOLT_SSD_H
#define VOLT_SSD_H

#include<stdint.h>

typedef struct LnvmVoltParams {
    uint8_t     num_ch;
    uint8_t     num_lun;
    uint16_t    num_blk;
    uint16_t    num_pg;
} LnvmVoltParams;

typedef struct LnvmVoltStatus {
    uint8_t     ready; // 0-busy, 1-ready to use
    uint8_t     active; // 0-disabled, 1-activated
} LnvmVoltStatus;

typedef struct LnvmVoltCtrl {
    LnvmVoltParams params;
    LnvmVoltStatus status;
} LnvmVoltCtrl;

/* TODO: after the work is done, change void to NvmeCtrl */
void nvme_volt_init(void *ctrl);
void * nvme_volt_main(void *ctrl);

#endif /* VOLT_SSD_H */ 
