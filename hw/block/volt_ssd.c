#include <stdio.h>
#include <pthread.h>
#include "volt_ssd.h"
#include "nvme.h"

void * nvme_volt_main(void *ctrl) {
    NvmeCtrl *n = (NvmeCtrl *) ctrl;
    LnvmVoltCtrl *volt = &n->volt_ctrl;
    
    while(volt->status.active){     
        printf("\nvolt: I am alive, I yield here each 10 minutes!\n");
        sleep(600);
    }
    return n;
}

static uint64_t nvme_volt_add_mem(LnvmVoltCtrl *volt, uint64_t bytes){
    volt->status.allocated_memory += bytes;
    return bytes;
}

static LnvmVoltPage * nvme_volt_init_page(LnvmVoltPage *pg){
    pg->state = 0; /* free */
    
    return ++pg;
}

static int nvme_volt_init_blocks(LnvmVoltCtrl *volt){    
    int page_count = 0;
    int i_blk;
    int i_pg;
    int total_blk = volt->params.num_blk*volt->params.num_lun;
    
    volt->blocks = g_malloc(nvme_volt_add_mem(volt, sizeof(LnvmVoltBlock)*total_blk));
    
    for(i_blk = 0; i_blk < total_blk; i_blk++){        
        LnvmVoltBlock *blk = &volt->blocks[i_blk];      
        blk->life = LNVM_VOLT_BLK_LIFE; 
        blk->pages = g_malloc(nvme_volt_add_mem(volt, sizeof(LnvmVoltPage)*volt->params.num_pg));
        blk->data = g_malloc0(nvme_volt_add_mem(volt, volt->params.pg_size*volt->params.num_pg));
        
        LnvmVoltPage *pg= blk->pages;
        for(i_pg = 0; i_pg < volt->params.num_pg; i_pg++){
            pg = nvme_volt_init_page(pg);     
            blk->data[i_pg*volt->params.pg_size] = (i_blk+1)*i_pg;
            page_count++;
        }
    }
    return page_count;
}

static void nvme_volt_init_luns(LnvmVoltCtrl *volt){
    int i_lun;
    volt->luns = g_malloc(nvme_volt_add_mem(volt,sizeof(LnvmVoltLun)*volt->params.num_lun));
    
    for(i_lun = 0; i_lun < volt->params.num_lun; i_lun++){
        volt->luns[i_lun].blk_offset = &volt->blocks[i_lun*volt->params.num_blk];
    }
}

void nvme_volt_init(void *ctrl)
{
    NvmeCtrl *n = (NvmeCtrl *) ctrl;
    LnvmCtrl *ln;
    LnvmIdGroup *c;
    LnvmVoltCtrl *volt;
    
    ln = &n->lightnvm_ctrl;
    c = &ln->id_ctrl.groups[0];
    volt = &n->volt_ctrl;
    
    volt->params.num_ch = c->num_ch;
    volt->params.num_lun = c->num_lun;
    volt->params.num_blk = c->num_blk;
    volt->params.num_pg = c->num_pg;
    volt->params.pg_size = c->fpg_sz;
    volt->status.active = 1; /* activated */
    volt->status.ready = 0; /* busy */
   
    /* Memory allocation. For now only LUNs, blocks and pages */    
    int pages_ok = nvme_volt_init_blocks(volt);
    nvme_volt_init_luns(volt);
    
    printf("\nvolt: Volatile SSD started succesfully with %d pages.\n", pages_ok);
    printf("volt: page_size: %d\n",(int)volt->params.pg_size);
    printf("volt: pages_per_block: %d\n",volt->params.num_pg);
    printf("volt: blocks_per_lun: %d\n",volt->params.num_blk);
    printf("volt: luns: %d\n",volt->params.num_lun);
    printf("volt: total_blocks: %d\n",volt->params.num_lun*volt->params.num_blk);   
    printf("volt: Volatile memory usage: %lu Mb\n",volt->status.allocated_memory/1048576);
        
    //printf("Data: %d\n",volt->luns[1].blk_offset[3].data[3*volt->params.pg_size]);
    
    pthread_t pth;
    int res = pthread_create(&pth, NULL, nvme_volt_main, n);
    if (res)
        volt->status.active = 0; /* disabled */
}