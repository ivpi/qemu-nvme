#include <stdio.h>
#include <pthread.h>
#include "volt_ssd.h"
#include "nvme.h"

void * nvme_volt_main(void *ctrl) {
    NvmeCtrl *n = (NvmeCtrl *) ctrl;
    LnvmVoltCtrl *volt = &n->volt_ctrl;
    printf("volt: Volatile SSD started succesfully!\n");
    while(volt->status.active){
        printf("\nvolt: I am alive!\n");
        printf("%d channel(s)\n%d lun(s) per channel\n%d block(s) per lun\n%d page(s) per block.\n",
                volt->params.num_ch, volt->params.num_lun, volt->params.num_blk, volt->params.num_pg);
        sleep(2);
    }
    return n;
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
    volt->status.active = 1; // activated
    volt->status.ready = 0; // busy
    
    pthread_t pth;
    int res = pthread_create(&pth, NULL, nvme_volt_main, n);
    if (res)
        volt->status.active = 0; //disabled
}