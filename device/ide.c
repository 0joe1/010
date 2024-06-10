#include "ide.h"
#include "stdio-kernel.h"
#include "global.h"
#include "stdio.h"
#include "debug.h"
#include "io.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"

struct channel channel[2];

#define reg_data(channel)     (channel->port_base + 0)
#define reg_err_fea(channel)  (channel->port_base + 1)
#define reg_seccnt(channel)   (channel->port_base + 2)
#define reg_lba_low(channel)  (channel->port_base + 3)
#define reg_lba_mid(channel)  (channel->port_base + 4)
#define reg_lba_high(channel) (channel->port_base + 5)
#define reg_device(channel)   (channel->port_base + 6)
#define reg_status(channel)   (channel->port_base + 7)
#define reg_control(channel)  (channel->port_base + 0x206)

#define DEVICE_MBS 0xa0
#define DEVICE_LBA 0x40
#define DEVICE_MASTER 0x00
#define DEVICE_SLAVE  0x10

#define CMD_IDENTIFY 0xec
#define CMD_READ     0x20
#define CMD_WRITE    0x30

#define STAT_BSY     0x80
#define STAT_DRDY    0x40
#define STAT_DRQ     0x08

uint_32 base_lba;
uint_32 pno,lno;
struct list partition_list;

static Bool busy_wait(struct disk* hd)
{
    uint_32 time_lim = 30*1000;
    while (time_lim -= 100)
    {
        if ((inb(reg_status(hd->chnl)) & STAT_BSY) == 0) {
            uint_8 stat = inb(reg_status(hd->chnl));
            return stat & (STAT_DRQ | STAT_DRDY);
        }
        sleep_ms(100);
    }
    return false;
}

void select_disk(struct disk* hd)
{
    uint_8 dev = hd->dev_no==0 ? DEVICE_MASTER : DEVICE_SLAVE;
    outb(reg_device(hd->chnl), \
         DEVICE_MBS | dev | DEVICE_LBA);
}
void select_sector(struct disk* hd,uint_32 start_lba,uint_8 sec_cnt)
{
    outb(reg_lba_low(hd->chnl),start_lba);
    outb(reg_lba_mid(hd->chnl),start_lba >> 8);
    outb(reg_lba_high(hd->chnl),start_lba >> 16); //16-23

    uint_8 origin = (hd->dev_no==0 ? DEVICE_MASTER : DEVICE_SLAVE) | \
                    DEVICE_MBS | DEVICE_LBA ;
    outb(reg_device(hd->chnl),origin | start_lba>>24);

    outb(reg_seccnt(hd->chnl),sec_cnt);
}

void cmd_out(struct disk* hd,uint_8 cmd) {
    hd->chnl->expecting_intr = true;
    outb(reg_status(hd->chnl),cmd);
}

void swap_pair_bytes(const char* src,char* dst,uint_32 len)
{
    uint_32 swap_times = len / 2;
    for (uint_32 pair=0 ; pair<swap_times ; pair++)
    {
        *(dst+1) = *(src++);
        *dst     = *(src++);

        dst += 2;
    }
    dst[len] = '\0';
}

void ide_read(struct disk* hd,void* buf,uint_32 start_lba,uint_32 sec_cnt)
{
    ASSERT(start_lba+sec_cnt <= max_lba);
    lock_acquire(&hd->chnl->lock);
    select_disk(hd);
    uint_32 lba = start_lba,read_done = 0;
    uint_32 sec_op;
    while (read_done < sec_cnt)
    {
        if ((read_done+256) <= sec_cnt) {
            sec_op = 256;
        } else {
            sec_op = sec_cnt - read_done;
        }
        select_sector(hd,lba,sec_op);
        cmd_out(hd,CMD_READ);
        sema_down(&hd->chnl->wait);
        if (!busy_wait(hd)){
            char error[20];
            sprintf(error,"%s:busy wait failed",hd->name);
            PANIC(error);
        }
        insw(reg_data(hd->chnl),(void*)((uint_32)buf+read_done*512),sec_op*256);
        read_done += sec_op;
        lba = start_lba + read_done;
    }
    lock_release(&hd->chnl->lock);
}

void ide_write(struct disk* hd,void* buf,uint_32 start_lba,uint_32 sec_cnt)
{
    ASSERT(start_lba+sec_cnt <= max_lba);
    lock_acquire(&hd->chnl->lock);
    select_disk(hd);
    uint_32 write_done = 0;
    uint_32 sec_op;
    while (write_done < sec_cnt)
    {
        if ((write_done+256) <= sec_cnt) {
            sec_op = 256;
        } else {
            sec_op = sec_cnt - write_done;
        }

        select_sector(hd,start_lba+write_done,sec_op);
        cmd_out(hd,CMD_WRITE);
        if (!busy_wait(hd)){
            char error[20];
            sprintf(error,"%s:busy wait failed",hd->name);
            PANIC(error);
        }
        outsw(reg_data(hd->chnl),(void*)((uint_32)buf+write_done*512),sec_op*256);
        sema_down(&hd->chnl->wait);
        write_done += sec_op;
    }
    lock_release(&hd->chnl->lock);
}

void identify_disk(struct disk* hd)
{
    char info[512];
    ASSERT(hd != NULL);
    lock_acquire(&hd->chnl->lock);
    select_disk(hd);
    cmd_out(hd,CMD_IDENTIFY);
    sema_down(&hd->chnl->wait);
    if (!busy_wait(hd)){
        char error[20];
        sprintf(error,"%s:busy wait failed",hd->name);
        PANIC(error);
    }
    insw(reg_data(hd->chnl),info,512/2); //读的是字，应该除以2

    char buf[43]; uint_32 sectors;
    uint_32 sn_st=10*2,mod_st=27*2,sec_st=60*2;
    memset(buf,0,sizeof(buf));
    swap_pair_bytes(info+sn_st,buf,20);
    printk("     disk sda info:\n");
    printk("        SN: %s\n",buf);
    memset(buf,0,sizeof(buf));
    swap_pair_bytes(info+mod_st,buf,40);
    printk("        MODULE: %s\n",buf);
    sectors = *(uint_32*)&info[sec_st];
    printk("        SECTORS: %d\n",sectors);
    printk("        CAPACITY: %dMB\n",sectors*512/1024/1024);
    lock_release(&hd->chnl->lock);
}

void ide_init(void)
{
    printk("ide init start\n");
    list_init(&partition_list);
    uint_8 hd_cnt = *(uint_8*)0x475;
    uint_32 chan_cnt = DIV_ROUND_UP(hd_cnt,2);

    uint_32 chan_no = 0;
    while (chan_no < chan_cnt)
    {
        struct channel* c = &channel[chan_no];
        if (chan_no == 0)
        {
            c->port_base = 0x1f0;
            c->int_no    = 0x20 + 14;
        }
        else
        {
            c->port_base = 0x170;
            c->int_no    = 0x20 + 15;
        }
        sema_init(&c->wait,0);
        lock_init(&c->lock);
        c->expecting_intr = false;
        register_handler(c->int_no,intr_hd_handler);

        struct disk* hd;
        for (uint_32 dev_no = 0 ; dev_no < 2 ; dev_no++)
        {
            hd = &c->disk[dev_no];
            hd->dev_no = dev_no;
            sprintf(hd->name,"sd%c",'a'+chan_no*2+dev_no);
            hd->chnl = c;
            if (dev_no != 0) {
                partition_scan(hd,0);
            }

            identify_disk(hd);
        }
        chan_no++;
    }
    printk("all partition info:\n");
    list_traversal(&partition_list,partition_info,NULL);
    printk("ide init done\n");
}

void intr_hd_handler(uint_32 int_num) //kernel.S压入中断号,call压入返回地址
{
    ASSERT(int_num == 0x2e || int_num == 0x2f);
    struct channel* c = &channel[int_num - 0x2e];
    if (c->expecting_intr)
    {
        c->expecting_intr = false;
        sema_up(&c->wait);
        inb(reg_status(c));
    }
}

void partition_scan(struct disk* hd,uint_32 lba)
{
    struct boot_record* br = sys_malloc(512);
    ide_read(hd,br,lba,1);
    struct partition_table_entry* pe;
    for (uint_32 part_idx = 0 ; part_idx < 4 ; part_idx++)
    {
        pe = &br->part_entry[part_idx];
        if (pe->par_type == 0x05)
        {
            if (base_lba == 0) {
                base_lba = pe->start_lba;
                partition_scan(hd,pe->start_lba);
            } else {
                partition_scan(hd,base_lba+pe->start_lba);
            }
        }
        else if (pe->par_type != 0)
        {
            struct partition* p;
            if (base_lba == 0)
            {
                p = &hd->primary[pno];
                sprintf(p->name,"%s%d",hd->name,1+pno);
                p->start_sec = pe->start_lba;
                p->sec_cnt   = pe->sec_cnt;
                pno++;
            }
            else 
            {
                p = &hd->logic[lno];
                sprintf(p->name,"%s%d",hd->name,5+lno);
                p->start_sec = lba + pe->start_lba;
                p->sec_cnt   = pe->sec_cnt;
                lno++;
            }
            p->hd = hd;
            list_append(&partition_list,&p->part_elm);
        }
    }
    sys_free(br);
}

Bool partition_info(struct list_elm* ptelm,int arg)
{
    struct partition* p = mem2entry(struct partition,ptelm,part_elm);
    printk("%s start_lba:%d, sec_cnt:%d\n",p->name,p->start_sec,p->sec_cnt);
    return false;
}
