// Build: gcc -O2 -std=c17 -Wall -Wextra mkfs_builder.c -o mkfs_builder
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#define BS 4096u               // block size
#define INODE_SIZE 128u
#define ROOT_INO 1u
#define DIRECT_MAX 12

uint64_t g_random_seed = 0; // This should be replaced by seed value from the CLI.

// below contains some basic structures you need for your project
// you are free to create more structures as you require

#pragma pack(push, 1)
typedef struct {
    // CREATE YOUR SUPERBLOCK HERE
    // ADD ALL FIELDS AS PROVIDED BY THE SPECIFICATION
    uint32_t magic;              
    uint32_t version;
    uint32_t block_size;
    uint64_t total_blocks;
    uint64_t inode_count;
    uint64_t inode_bitmap_start;
    uint64_t inode_bitmap_blocks;
    uint64_t data_bitmap_start;
    uint64_t data_bitmap_blocks;
    uint64_t inode_table_start;
    uint64_t inode_table_blocks;
    uint64_t data_region_start;
    uint64_t data_region_blocks;
    uint64_t root_inode;
    uint64_t mtime_epoch;
    uint32_t flags;    
    // THIS FIELD SHOULD STAY AT THE END
    // ALL OTHER FIELDS SHOULD BE ABOVE THIS
    uint32_t checksum;            // crc32(superblock[0..4091])
} superblock_t;
#pragma pack(pop)
_Static_assert(sizeof(superblock_t) == 116, "superblock must fit in one block");

#pragma pack(push,1)
typedef struct {
    // CREATE YOUR INODE HERE
    // IF CREATED CORRECTLY, THE STATIC_ASSERT ERROR SHOULD BE GONE
    uint16_t mode;
    uint16_t links;
    uint32_t uid;
    uint32_t gid;
    uint64_t size_bytes;
    uint64_t atime, mtime, ctime;
    uint32_t direct[DIRECT_MAX];
    uint32_t reserved0, reserved1, reserved2;
    uint32_t proj_id;
    uint32_t uid16_gid16;
    uint64_t xattr_ptr;
    // THIS FIELD SHOULD STAY AT THE END
    // ALL OTHER FIELDS SHOULD BE ABOVE THIS
    uint64_t inode_crc;   // low 4 bytes store crc32 of bytes [0..119]; high 4 bytes 0

} inode_t;
#pragma pack(pop)
_Static_assert(sizeof(inode_t)==INODE_SIZE, "inode size mismatch");

#pragma pack(push,1)
typedef struct {
    // CREATE YOUR DIRECTORY ENTRY STRUCTURE HERE
    // IF CREATED CORRECTLY, THE STATIC_ASSERT ERROR SHOULD BE GONE
    uint32_t inode_no;
    uint8_t type;
    char name[58];
    uint8_t  checksum; // XOR of bytes 0..62
} dirent64_t;
#pragma pack(pop)
_Static_assert(sizeof(dirent64_t)==64, "dirent size mismatch");


// ==========================DO NOT CHANGE THIS PORTION=========================
// These functions are there for your help. You should refer to the specifications to see how you can use them.
// ====================================CRC32====================================
uint32_t CRC32_TAB[256];
void crc32_init(void){
    for (uint32_t i=0;i<256;i++){
        uint32_t c=i;
        for(int j=0;j<8;j++) c = (c&1)?(0xEDB88320u^(c>>1)):(c>>1);
        CRC32_TAB[i]=c;
    }
}
uint32_t crc32(const void* data, size_t n){
    const uint8_t* p=(const uint8_t*)data; uint32_t c=0xFFFFFFFFu;
    for(size_t i=0;i<n;i++) c = CRC32_TAB[(c^p[i])&0xFF] ^ (c>>8);
    return c ^ 0xFFFFFFFFu;
}
// ====================================CRC32====================================

// WARNING: CALL THIS ONLY AFTER ALL OTHER SUPERBLOCK ELEMENTS HAVE BEEN FINALIZED
static uint32_t superblock_crc_finalize(superblock_t *sb) {
    sb->checksum = 0;
    uint32_t s = crc32((void *) sb, BS - 4);
    sb->checksum = s;
    return s;
}

// WARNING: CALL THIS ONLY AFTER ALL OTHER SUPERBLOCK ELEMENTS HAVE BEEN FINALIZED
void inode_crc_finalize(inode_t* ino){
    uint8_t tmp[INODE_SIZE]; memcpy(tmp, ino, INODE_SIZE);
    // zero crc area before computing
    memset(&tmp[120], 0, 8);
    uint32_t c = crc32(tmp, 120);
    ino->inode_crc = (uint64_t)c; // low 4 bytes carry the crc
}

// WARNING: CALL THIS ONLY AFTER ALL OTHER SUPERBLOCK ELEMENTS HAVE BEEN FINALIZED
void dirent_checksum_finalize(dirent64_t* de) {
    const uint8_t* p = (const uint8_t*)de;
    uint8_t x = 0;
    for (int i = 0; i < 63; i++) x ^= p[i];   // covers ino(4) + type(1) + name(58)
    de->checksum = x;
}

static void set_bit(uint8_t *bm, uint64_t idx) {
    bm[idx >> 3] |= (1u << (idx & 7));
}

typedef struct {
    const char *image;
    long size_kib;
    long inodes;
} cli_t;

static cli_t parse_cli(int argc, char **argv) {
    cli_t c = {0};
    if (argc != 7) {
        fprintf(stderr,
            "Usage: %s --image <out.img> --size-kib <180..4096> --inodes <128..512>\n",
            argv[0]);
        exit(1);
    }
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--image") == 0 && i+1 < argc) {
            c.image = argv[++i];
        } else if (strcmp(argv[i], "--size-kib") == 0 && i+1 < argc) {
            c.size_kib = strtol(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--inodes") == 0 && i+1 < argc) {
            c.inodes = strtol(argv[++i], NULL, 10);
        }
    }
    if (!c.image || c.size_kib < 180 || c.size_kib > 4096 || (c.size_kib % 4 != 0)) {
        fprintf(stderr, "Invalid --size-kib (must be 180..4096, multiple of 4)\n");
        exit(1);
    }
    if (c.inodes < 128 || c.inodes > 512) {
        fprintf(stderr, "Invalid --inodes (must be 128..512)\n");
        exit(1);
    }
    return c;
}

int main(int argc, char **argv) {
    crc32_init();
    // WRITE YOUR DRIVER CODE HERE
    // PARSE YOUR CLI PARAMETERS
    // THEN CREATE YOUR FILE SYSTEM WITH A ROOT DIRECTORY
    // THEN SAVE THE DATA INSIDE THE OUTPUT IMAGE
    cli_t cli = parse_cli(argc, argv);

    uint64_t total_blocks = (cli.size_kib * 1024u) / BS;
    uint64_t inode_table_blocks = (cli.inodes * INODE_SIZE + BS - 1) / BS;

    // layout
    uint64_t sb_start = 0;
    uint64_t inode_bitmap_start = 1;
    uint64_t data_bitmap_start  = 2;
    uint64_t inode_table_start  = 3;
    uint64_t data_region_start  = inode_table_start + inode_table_blocks;
    if (data_region_start >= total_blocks) {
        fprintf(stderr, "Image too small for metadata\n");
        return 1;
    }
    uint64_t data_region_blocks = total_blocks - data_region_start;

    size_t image_bytes = total_blocks * BS;
    uint8_t *image = calloc(1, image_bytes);
    if (!image) { perror("calloc"); return 1; }

    superblock_t *sb = (superblock_t*)(image + sb_start * BS);
    sb->magic = 0x4D565346u;
    sb->version = 1;
    sb->block_size = BS;
    sb->total_blocks = total_blocks;
    sb->inode_count = cli.inodes;
    sb->inode_bitmap_start = inode_bitmap_start;
    sb->inode_bitmap_blocks = 1;
    sb->data_bitmap_start = data_bitmap_start;
    sb->data_bitmap_blocks = 1;
    sb->inode_table_start = inode_table_start;
    sb->inode_table_blocks = inode_table_blocks;
    sb->data_region_start = data_region_start;
    sb->data_region_blocks = data_region_blocks;
    sb->root_inode = ROOT_INO;
    sb->mtime_epoch = (uint64_t)time(NULL);
    sb->flags = 0;
    superblock_crc_finalize(sb);

    uint8_t *inode_bm = image + inode_bitmap_start * BS;
    uint8_t *data_bm  = image + data_bitmap_start  * BS;
    set_bit(inode_bm, 0); 
    set_bit(data_bm,  0); 

    inode_t *inode_tbl = (inode_t*)(image + inode_table_start * BS);
    inode_t *root = &inode_tbl[0];
    memset(root, 0, sizeof(*root));
    root->mode = 040000;  
    root->links = 2;
    root->uid = 0;
    root->gid = 0;
    root->size_bytes = 64 * 2;
    root->atime = root->mtime = root->ctime = sb->mtime_epoch;
    root->direct[0] = (uint32_t)data_region_start;
    root->proj_id = 0;
    inode_crc_finalize(root);

    uint8_t *data_region = image + data_region_start * BS;
    dirent64_t de;
    memset(&de, 0, sizeof(de));
    de.inode_no = ROOT_INO; de.type = 2; strncpy(de.name, ".", sizeof(de.name)-1);
    dirent_checksum_finalize(&de);
    memcpy(data_region, &de, sizeof(de));
    memset(&de, 0, sizeof(de));
    de.inode_no = ROOT_INO; de.type = 2; strncpy(de.name, "..", sizeof(de.name)-1);
    dirent_checksum_finalize(&de);
    memcpy(data_region + 64, &de, sizeof(de));

    FILE *out = fopen(cli.image, "wb");
    if (!out) { perror("fopen"); free(image); return 1; }
    if (fwrite(image, 1, image_bytes, out) != image_bytes) {
        perror("fwrite"); fclose(out); free(image); return 1;
    }
    fclose(out);
    free(image);
    return 0;
}

