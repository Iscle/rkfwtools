#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/syslimits.h>
#include <string.h>
#include <errno.h>

static int write_file(char *file, uint8_t *buf, uint32_t len) {
    int ret = -1;
    int outfd = -1;
    uint8_t *outbuf = MAP_FAILED;

    printf("Writing file %s\n", file);

    // Create folders if required
    char *sep = file;
    while ((sep = strchr(sep, '/')) != NULL) {
        char tmp[PATH_MAX];
        memcpy(tmp, file, sep - file);
        tmp[sep - file] = '\0';
        if (tmp[0] == '\0') {
            sep++;
            continue;
        }
        if (mkdir(tmp, 0755) == -1 && errno != EEXIST) {
            printf("%s: %s\n", tmp, strerror(errno));
            goto exit;
        }
        sep++;
    }

    outfd = open(file, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (outfd < 0) {
        printf("Error opening output file\n");
        goto exit;
    }

    if (ftruncate(outfd, len) < 0) {
        printf("Error truncating output file\n");
        goto exit;
    }

    outbuf = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, outfd, 0);
    if (outbuf == MAP_FAILED) {
        printf("Error mapping output file\n");
        goto exit;
    }

    memcpy(outbuf, buf, len);

    ret = 0;
exit:
    if (outbuf != MAP_FAILED) {
        munmap(outbuf, len);
    }
    if (outfd >= 0) {
        close(outfd);
    }
    return ret;
}

int unpack_rkaf(char *outfile, uint8_t *buf, uint32_t len) {
    int ret = -1;
    char tmp[PATH_MAX];

    // Magic
    if (memcmp(buf, "RKAF", 4) != 0) {
        printf("Error: invalid magic\n");
        goto exit;
    }

    uint32_t file_size = *(uint32_t *) (buf + 0x04) + 4;
    if (file_size != len) {
        printf("Error: invalid file size (%x != %x)\n", file_size, len);
        goto exit;
    }

    printf("Model: %s\n", buf + 0x08);
    printf("Manufacturer: %s\n", buf + 0x48);

    uint32_t file_count  = *(uint32_t *) (buf + 0x88);
    printf("File count: %d\n", file_count);

    for (uint32_t i = 0; i < file_count; i++) {
        uint8_t *filebuf = buf + 0x8C + i * 0x70;
        char *name = (char *) (filebuf);
        char *path = (char *) (filebuf + 0x20);
        uint32_t offset = *(uint32_t *) (filebuf + 0x60);
        uint32_t size = *(uint32_t *) (filebuf + 0x6C);
        printf("File %d: %s (%s) @ 0x%08X (%d)\n", i, name, path, offset, size);

        snprintf(tmp, sizeof(tmp), "%s/%s", outfile, path);
        if (write_file(tmp, buf + offset, size) < 0) {
            printf("Error writing file\n");
            goto exit;
        }
    }

    ret = 0;
exit:
    return ret;
}

int unpack_rkfw(char *infile, char *outfile) {
    int ret = -1;
    int infd = -1;
    struct stat st;
    uint8_t *inbuf = MAP_FAILED;
    char tmp[PATH_MAX];

    infd = open(infile, O_RDONLY);
    if (infd < 0) {
        printf("Error opening input file\n");
        goto exit;
    }

    if (fstat(infd, &st) < 0) {
        printf("Error getting input file size\n");
        goto exit;
    }

    inbuf = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, infd, 0);
    if (inbuf == MAP_FAILED) {
        printf("Error mapping input file\n");
        goto exit;
    }

    // Magic
    if (memcmp(inbuf, "RKFW", 4) != 0) {
        printf("Error: invalid magic\n");
        goto exit;
    }

    uint32_t boot_offset = *(uint32_t *) (inbuf + 0x19);
    uint32_t boot_size = *(uint32_t *) (inbuf + 0x1D);
    uint32_t update_offset = *(uint32_t *) (inbuf + 0x21);
    uint32_t update_size = *(uint32_t *) (inbuf + 0x25);
    printf("Boot offset: 0x%08X\n", boot_offset);
    printf("Boot size: %d\n", boot_size);
    printf("Update offset: 0x%08X\n", update_offset);
    printf("Update size: %d\n", update_size);

//    snprintf(tmp, sizeof(tmp), "%s/%s", outfile, "boot.img");
//    if (write_file(tmp, inbuf + boot_offset, boot_size) < 0) {
//        printf("Error writing boot image\n");
//        goto exit;
//    }
//
//    snprintf(tmp, sizeof(tmp), "%s/%s", outfile, "update.img");
//    if (write_file(tmp, inbuf + update_offset, update_size) < 0) {
//        printf("Error writing update image\n");
//        goto exit;
//    }

    unpack_rkaf(outfile, inbuf + update_offset, update_size);

    ret = 0;
exit:
    if (infd != -1) {
        close(infd);
    }
    if (inbuf != MAP_FAILED) {
        munmap(inbuf, st.st_size);
    }
    return ret;
}

int pack(char *outfile, char *infolder) {

    return 0;
}

int main(int argc, const char * argv[]) {
    printf("Hello, World!\n");

    unpack_rkfw("/Users/iscle/Downloads/XB-RK3399D26-V1.0-8821C-BZ-EN-01Y215-180D-EDITION11_20221125.img",
                "/Users/iscle/Downloads/extracted");

    return 0;
}
