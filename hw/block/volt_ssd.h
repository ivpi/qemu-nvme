#ifndef VOLT_SSD_H
#define VOLT_SSD_H

#include <stdint.h>
#include "include/qemu/timer.h"
#include "block/coroutine.h"
#include "block/aio.h"
#include "block/block.h"
#include "block/block_int.h"
#include "sysemu/block-backend.h"

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

typedef struct LnvmVoltBlockAIOCBCoroutine {
    BlockAIOCB common;
    BlockRequest req;
    bool is_write;
    bool *done;
    QEMUBH* bh;
} LnvmVoltBlockAIOCBCoroutine;

struct BlockBackend {
    char *name;
    int refcnt;
    BlockDriverState *bs;
    DriveInfo *legacy_dinfo;
    QTAILQ_ENTRY(BlockBackend) link;
    void *dev;
    const BlockDevOps *dev_ops;
    void *dev_opaque;
};

void nvme_volt_init(struct NvmeCtrl *n);
void nvme_volt_main(void *ctrl); /* main thread */
void coroutine_fn nvme_volt_redirect_co(void *opaque);
BlockAIOCB *nvme_volt_redirect_read(BlockBackend *blk, int64_t sector_num,
                           QEMUIOVector *iov, int nb_sectors,
                           BlockCompletionFunc *cb, void *opaque);
BlockAIOCB *nvme_volt_redirect_write(BlockBackend *blk, int64_t sector_num,
                           QEMUIOVector *iov, int nb_sectors,
                           BlockCompletionFunc *cb, void *opaque);
BlockAIOCB *nvme_volt_dma_blk_read_list(BlockBackend *blk,
                         QEMUSGList *sg, uint64_t *sector_list,
                         void (*cb)(void *opaque, int ret), void *opaque);
BlockAIOCB *nvme_volt_dma_blk_write_list(BlockBackend *blk,
                         QEMUSGList *sg, uint64_t *sector_list,
                         void (*cb)(void *opaque, int ret), void *opaque);

#endif /* VOLT_SSD_H */ 