#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define SECTOR_SIZE     512

struct _partition {
    uint8_t      status;
    uint8_t      chs_start[3];
    uint8_t      part_type;
    uint8_t      chs_end[3];
    uint32_t     lba_start;
    uint32_t     sectors;
};

void print_mbr(unsigned char* mbr) {
    for (size_t i = 0; i < SECTOR_SIZE; i++) {
        printf("%.2x ", mbr[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n");
}

void check_file_validity(char* fd) {
    if(!access(fd, F_OK) == 0) {
        printf("File doesn't exist\n");
        exit(EXIT_FAILURE);
    }
    if (!access(fd, R_OK) == 0) {
        printf("No read permissions\n");
        exit(EXIT_FAILURE);
    }
}

void analyze(unsigned char* mbr) {
    unsigned char magic_num[2];
    magic_num[0] = 0x55; magic_num[1] = 0xaa;
    if (strncmp((const char*) (mbr + 510), (const char*) magic_num, 2) != 0) {
        printf("Not a MBR\n");
        exit(EXIT_FAILURE);
    }
    printf("Yay\n"); /*TODO: add analysis here */
}

int main(int argc, char *argv[]) {
    unsigned char mbr[SECTOR_SIZE];
    int fd;
    if(argc == 2) {
        check_file_validity(argv[1]);
        fd = open(argv[1], O_RDONLY);
        lseek(fd, 0, SEEK_SET); /* Offset 0 */
        read(fd, mbr, SECTOR_SIZE);
        print_mbr(mbr);
        printf("\nANALYSIS:\n");
        analyze(mbr);
    } else {
        printf("Incorrect usage: lsmbr <disk>\n");
    }
}