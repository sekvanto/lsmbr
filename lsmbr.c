/* 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "pt-mbr-partnames.h"

#define VERSION         1
#define SUBVERSION      0
#define SECTOR_SIZE     512

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

int is_gpt = 0;

struct _partition {
    uint8_t      status;
    uint8_t      chs_start[3];
    uint8_t      part_type;
    uint8_t      chs_end[3];
    uint32_t     lba_start;
    uint32_t     sectors;
};

void show_bytes(unsigned char* mbr, int length) {
    for (size_t i = 0; i < length; i++) {
        printf("%.2x ", mbr[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n");
}

void check_file_validity(char* fd) {
    if (!access(fd, F_OK) == 0) {
        printf("File doesn't exist\n");
        exit(EXIT_FAILURE);
    }
    if (!access(fd, R_OK) == 0) {
        printf("No read permissions\n");
        exit(EXIT_FAILURE);
    }
}

void mbr_bootstrap(unsigned char* mbr, int length) {
    printf("%sBootstrap code area%s", MAGENTA, RESET);
    for (size_t i = 0; i < length; i++) {
        if (mbr[i] != 0) {
            printf("%s - disklabel type: DOS%s\n", GREEN, RESET);
            break;
        }
        if (i == length - 1) {
            printf("%s - disklabel type: GPT%s\n", GREEN, RESET);
            is_gpt = 1;
        }
    }
    show_bytes(mbr, length);
    if (!is_gpt) {
        printf("\n%sEnter 'y' to see raw binary: %s", GREEN, RESET);
        char raw;
        scanf("%c", &raw);
        if (raw == 'y') {
            write(STDOUT_FILENO, mbr, length);
            printf("\n");
        }  
    }
}

/* chs[3] in this order: head (8 bits), sector (6 bits), cylinder (10 bits) */
void show_chs_coords(uint8_t* chs, char* id) {
    uint8_t  sector   = chs[1] & 0b00111111;
    uint16_t cylinder = ((chs[1] & 0b11000000) << 2) | chs[2];
    printf("%s head, sector, cylinder coordinates: ", id);
    printf("%s%.2x, %.2x, %.2x%s\n", RED, chs[0], sector, cylinder, RESET);
}

void show_type(uint8_t id) {
    char* type = "";
    for (size_t i = 0; pt_types[i].type != 0; i++)
        if (pt_types[i].id == id)
            type = pt_types[i].type;
    printf("Id: %s%x%s, type: %s\n", RED, id, RESET, type);
}

void partition_records(unsigned char* mbr, int part_length, int part_num) {
    for (size_t i = 0; i < part_num; i++) {
        struct _partition* p = (void *) (mbr + i * part_length);
        printf("\n%s<Partition %ld>%s", BOLDYELLOW, i, RESET);
        /* Sector coordinate can't be 0 */
        if ((p->chs_start[1] & 0b00111111) == 0) {
            printf(" - doesn't exist\n");
            continue;
        }
        char *bootable = p->status == 0 ? " not" : ""; 
        printf("\nBoot indicator: %s%.2x%s -%s bootable\n", RED, p->status, RESET, bootable);
        show_chs_coords(p->chs_start, "Starting");
        show_type(p->part_type);
        show_chs_coords(p->chs_end, "Ending");
        printf("Logical block address of first absolute sector in the partition: %s%.2x%s\n", RED, p->lba_start, RESET);
        printf("Number of sectors in partition: %s%.2x%s\n", RED, p->sectors, RESET);
    }
    printf("\n");
}

void analyze(unsigned char* mbr) {
    unsigned char magic_num[2];
    magic_num[0] = 0x55; magic_num[1] = 0xaa;
    if (strncmp((const char*) (mbr + 510), (const char*) magic_num, 2) != 0) {
        printf("Not a MBR\n");
        exit(EXIT_FAILURE);
    }
    mbr_bootstrap(mbr, 440);
    printf("\n%sDisk identifier: %s", MAGENTA, RESET);
    show_bytes((mbr+440), 4);
    printf("%sUnknown: %s", MAGENTA, RESET);
    show_bytes((mbr+444), 2);
    partition_records((mbr+446), 16, 4);
    printf("%sSignature: %s", MAGENTA, RESET);
    show_bytes((mbr+510), 2);
}

int main(int argc, char *argv[]) {
    unsigned char mbr[SECTOR_SIZE];
    int fd;
    if(argc == 2) {
        check_file_validity(argv[1]);
        fd = open(argv[1], O_RDONLY);
        lseek(fd, 0, SEEK_SET); /* Offset 0 */
        read(fd, mbr, SECTOR_SIZE);
        printf("%sMBR analyzer, v%d.%d:%s\n", RED, VERSION, SUBVERSION, RESET);
        analyze(mbr);
        printf("\n%sEnter 'y' to see full MBR sector: %s", GREEN, RESET);
        char full;
        scanf(" %c", &full);
        if (full == 'y')
            show_bytes(mbr, SECTOR_SIZE);
    } else {
        printf("Incorrect usage: lsmbr <disk>\n");
    }
}
