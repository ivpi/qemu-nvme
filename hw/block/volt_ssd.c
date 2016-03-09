#include <stdio.h>
#include <pthread.h>
#include "volt_ssd.h"
#include "nvme.h"

void * nvme_volt_main(void *ctrl) {
    NvmeCtrl *n = (NvmeCtrl *) ctrl;
    LnvmVoltCtrl *volt = &n->volt_ctrl;
    
    while(volt->status.active){     
        printf("\nvolt: I am alive!\n");
        printf("%d channel(s)\n%d lun(s) per channel\n%d block(s) per lun\n%d page(s) per block\nPage size: %d\n",
            volt->params.num_ch, volt->params.num_lun, volt->params.num_blk, volt->params.num_pg, (int) volt->params.pg_size);
        sleep(2000);
    }
    return n;
}

static int nvme_volt_init_page(int c){
    return ++c;
}

static int nvme_volt_init_blocks(LnvmVoltCtrl *volt){    
    /* for now, only LUNs*/
    int c = 0;
    int i_blk;
    int i_pg;
    int total_blk = volt->params.num_blk*volt->params.num_lun;
    for(i_blk = 0; i_blk <= total_blk; i_blk++){
        for(i_pg = 0; i_pg <= volt->params.num_blk; i_pg++){
            c = nvme_volt_init_page(c);
        }        
    }
    return c;
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
    
    /* Memory allocation */
    int init_pages = nvme_volt_init_blocks(volt);
    printf("\nvolt: Volatile SSD started succesfully with %d pages.\n", init_pages);
    
    pthread_t pth;
    int res = pthread_create(&pth, NULL, nvme_volt_main, n);
    if (res)
        volt->status.active = 0; /* disabled */
}