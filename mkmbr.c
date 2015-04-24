/* vim: et:sw=4:ts=4:sts=4:tw=80 */
/*
 * mkmbr
 * Simple tool for creating MBR partition table from command line
 *
 * Copyright(c) 2015, Mariusz Glebocki <mglb@arccos-1.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

static const char usage_str[] = "\
Usage: \n\
\n\
    mkmbr dev p1_start p1_sectors [p2_start p2_sectors [...]]\n\
\n\
Where:\n\
    dev        - block device path\n\
    pX_start   - first sector number or \"auto\" to use next free sector\n\
    pX_sectors - partition's sectors count or \"auto\" to use all free space\n\
\n";

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
# error Sorry, only little endian machines are supported for now
#endif

#define PACKED __attribute__((packed))

#define PARTITION_BOOTABLE_FLAG 0x80
#define PARTITION_TYPE          0x83    /* GNU/Linux */

struct PACKED partition_t {
    uint8_t     bootable_flag;
    uint8_t     first_sector_chs[3];
    uint8_t     type;
    uint8_t     last_sector_chs[3];
    uint32_t    first_sector_lba;
    uint32_t    sectors_num_lba;
};

#define MBR_BOOT_SIGNATURE      0xAA55

struct PACKED mbr_t {
    uint8_t             bootstrap[446];
    struct partition_t  partitions[4];
    uint16_t            boot_signature;
};

void
print_usage() {
    write(STDERR_FILENO, usage_str, sizeof(usage_str));
}

void
mbr_init(struct mbr_t *mbr) {
    memset(mbr, 0, sizeof(struct mbr_t));
    mbr->boot_signature = MBR_BOOT_SIGNATURE;
}

int
main(int argc, char *argv[]) {
    struct mbr_t    mbr;
    int             argn;
    int             partn       = 0;
    const char     *blkdev      = argv[1];
    int             blkdev_fd   = -1;
    unsigned long   blkdev_size = 0;
    int             ret;
    unsigned long   start;
    unsigned long   size;
    unsigned long   next        = 1;
    struct stat     st;

    if(argc < 4 || argc % 2 != 0) {
        print_usage();
        return EINVAL;
    }

    blkdev_fd = open(blkdev, O_RDWR);
    if(blkdev_fd < 0)
        return errno;

    ret = fstat(blkdev_fd, &st);
    if(ret < 0)
        goto err_exit;

    blkdev_size = st.st_size / 512;

    mbr_init(&mbr);
    for(argn = 2; argn < argc && partn < 4 && next < blkdev_size; argn += 2) {
        if(!strcmp(argv[argn], "auto") && next > 0) {
            start = next;
        } else {
            errno = 0;
            start = strtol(argv[argn], NULL, 0);
            if(errno)
                goto err_exit;
        }
        if(!strcmp(argv[argn + 1], "auto")) {
            size = blkdev_size - start;
        } else {
            errno = 0;
            size = strtol(argv[argn + 1], NULL, 0);
            if(errno)
                goto err_exit;
        }
        if(start < next || start + size > blkdev_size) {
            errno = EINVAL;
            goto err_exit;
        }
        mbr.partitions[partn].first_sector_lba  = (uint32_t)start;
        mbr.partitions[partn].sectors_num_lba   = (uint32_t)size;
        mbr.partitions[partn].type              = PARTITION_TYPE;
        next = start + size;
        ++partn;
    }
    ret = write(blkdev_fd, &mbr, sizeof(mbr));
    if(ret)
        goto err_exit;

    return 0;

err_exit:
    if(blkdev_fd >= 0)
        close(blkdev_fd);
    return errno;
}

