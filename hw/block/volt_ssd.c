#include <stdio.h>
#include <pthread.h>
#include "volt_ssd.h"
#include "nvme.h"

void nvme_volt_main(void *opaque)
{
    LnvmVoltCtrl *volt = (LnvmVoltCtrl *) opaque;

    if (volt->status.active) {
        printf("\nvolt: I am alive, I yield here each 10 minutes!\n");
        timer_mod(volt->mainTimer, qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) + LNVM_VOLT_SECOND * 600);
    }
}

static size_t nvme_volt_add_mem(LnvmVoltCtrl *volt, int64_t bytes)
{
    volt->status.allocated_memory += bytes;
    return bytes;
}

static void nvme_volt_sub_mem(LnvmVoltCtrl *volt, int64_t bytes)
{
    volt->status.allocated_memory -= bytes;
}

static LnvmVoltPage * nvme_volt_init_page(LnvmVoltPage *pg)
{
    pg->state = 0; /* free */
    return ++pg;
}

static int nvme_volt_init_blocks(LnvmVoltCtrl *volt)
{
    int page_count = 0;
    int i_blk;
    int i_pg;
    int total_blk = volt->params.num_blk * volt->params.num_lun;

    volt->blocks = g_malloc(nvme_volt_add_mem(volt, sizeof(LnvmVoltBlock) * total_blk));
    if (!volt->blocks)
        return LNVM_VOLT_MEM_ERROR;
               
    for (i_blk = 0; i_blk < total_blk; i_blk++) {
        LnvmVoltBlock *blk = &volt->blocks[i_blk];
        blk->life = LNVM_VOLT_BLK_LIFE;

        blk->pages = g_malloc(nvme_volt_add_mem(volt, sizeof(LnvmVoltPage) * volt->params.num_pg));
        if (!blk->pages)
            return LNVM_VOLT_MEM_ERROR;
        blk->next_pg = blk->pages;

        blk->data = g_malloc0(nvme_volt_add_mem(volt, volt->params.pg_size * volt->params.num_pg));
        if (!blk->data)
            return LNVM_VOLT_MEM_ERROR;

        LnvmVoltPage *pg = blk->pages;
        for (i_pg = 0; i_pg < volt->params.num_pg; i_pg++) {
            pg = nvme_volt_init_page(pg);
            blk->data[i_pg * volt->params.pg_size] = (i_blk + 1) * i_pg;
            page_count++;
        }
    }
    return page_count;
}

static int nvme_volt_init_luns(LnvmVoltCtrl *volt)
{
    int i_lun;
    volt->luns = g_malloc(nvme_volt_add_mem(volt, sizeof (LnvmVoltLun) * volt->params.num_lun));
    if (!volt->luns)
        return LNVM_VOLT_MEM_ERROR;

    for (i_lun = 0; i_lun < volt->params.num_lun; i_lun++) {
        volt->luns[i_lun].blk_offset = &volt->blocks[i_lun * volt->params.num_blk];
    }
    return LNVM_VOLT_MEM_OK;
}

static void nvme_volt_clean_mem(LnvmVoltCtrl *volt)
{
    int total_blk = volt->params.num_blk * volt->params.num_lun;
    int i;
    for (i = 0; i < total_blk; i++) {
        g_free(volt->blocks[i].data);
        nvme_volt_sub_mem(volt, volt->params.pg_size * volt->params.num_pg);
        g_free(volt->blocks[i].pages);
        nvme_volt_sub_mem(volt, sizeof (LnvmVoltPage) * volt->params.num_pg);
    }
    g_free(volt->blocks);
    nvme_volt_sub_mem(volt, sizeof (LnvmVoltBlock) * total_blk);

    g_free(volt->luns);
    nvme_volt_sub_mem(volt, sizeof (LnvmVoltLun) * volt->params.num_lun);
}

void nvme_volt_init(struct NvmeCtrl *n)
{
    LnvmCtrl *ln;
    LnvmIdGroup *c;
    LnvmVoltCtrl *volt;
    int i;

    n->volt_ctrl = g_malloc(sizeof (LnvmVoltCtrl) * n->num_namespaces);

    for (i = 0; i < n->num_namespaces; i++) {
        ln = &n->lightnvm_ctrl;
        c = &ln->id_ctrl.groups[0];
        volt = &n->volt_ctrl[i];
        
        volt->ctrl = n;
        volt->ns = &n->namespaces[i];
        volt->params.num_ch = c->num_ch;
        volt->params.num_lun = c->num_lun;
        volt->params.num_blk = c->num_blk;
        volt->params.num_pg = c->num_pg;
        volt->params.pg_size = c->fpg_sz;
        volt->status.active = 1; /* activated */
        volt->status.ready = 0; /* busy */
        volt->status.allocated_memory = 0;

        /* Memory allocation. For now only LUNs, blocks and pages */
        int pages_ok = nvme_volt_init_blocks(volt);
        int res = nvme_volt_init_luns(volt);

        if (!pages_ok || !res)
            goto MEM_CLEAN;

        volt->mainTimer = timer_new_us(QEMU_CLOCK_VIRTUAL, nvme_volt_main, volt);
        timer_mod(volt->mainTimer, qemu_clock_get_us(QEMU_CLOCK_VIRTUAL));
        volt->status.ready = 1; /* ready to use */

        printf("\nvolt: Volatile SSD ns%d started succesfully with %d pages.\n", volt->ns->id, pages_ok);
        printf("volt: page_size: %d\n", (int) volt->params.pg_size);
        printf("volt: pages_per_block: %d\n", volt->params.num_pg);
        printf("volt: blocks_per_lun: %d\n", volt->params.num_blk);
        printf("volt: luns: %d\n", volt->params.num_lun);
        printf("volt: total_blocks: %d\n", volt->params.num_lun * volt->params.num_blk);
        printf("volt: Volatile memory usage: %lu Mb\n", volt->status.allocated_memory / 1048576);
        //printf("Data: %d\n",volt->luns[1].blk_offset[3].data[3*volt->params.pg_size]);
        continue;

    MEM_CLEAN:
        volt->status.active = 0;
        volt->status.ready = 0;
        nvme_volt_clean_mem(volt);
        printf("volt: Not initialized ns%d! Memory allocation failed.\n", volt->ns->id);
        printf("volt: Volatile memory usage: %lu bytes.\n", volt->status.allocated_memory);
        continue;
    }
}