#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>
#include <assert.h>


#define BS 4096u
#define INODE_SIZE 128u
#define ROOT_INO 1u
#define DIRECT_MAX 12
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
    uint32_t reserved_0;
    uint32_t reserved_1;
    uint32_t reserved_2;
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
    uint8_t  type;               
    char     name[58];
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

static inline int test_bit(const uint8_t *bm, uint64_t idx) { return (bm[idx >> 3] >> (idx & 7)) & 1; }
static inline void set_bit(uint8_t *bm, uint64_t idx) { bm[idx >> 3] |= (uint8_t)(1u << (idx & 7)); }

static void die(const char *msg) { fprintf(stderr, "%s\n", msg); exit(1); }

typedef struct {
    const char *input;
    const char *output;
    const char *file;
} cli_t;

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage:\n"
        "  %s --input <in.img> --output <out.img> --file <path>\n",
        prog);
}

static cli_t parse_cli(int argc, char **argv) {
    cli_t c = {0};
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "--input") && i+1 < argc) { c.input = argv[++i]; continue; }
        if (!strcmp(a, "--output") && i+1 < argc) { c.output = argv[++i]; continue; }
        if (!strcmp(a, "--file") && i+1 < argc) { c.file = argv[++i]; continue; }
        if (!strncmp(a, "--input=", 8)) { c.input = a+8; continue; }
        if (!strncmp(a, "--output=", 9)) { c.output = a+9; continue; }
        if (!strncmp(a, "--file=", 7)) { c.file = a+7; continue; }
        die("Unknown/invalid argument");
    }
    if (!c.input || !c.output || !c.file) {
        usage(argv[0]); exit(2);
    }
    return c;
}
int main(int argc, char **argv) {
    crc32_init();
    // WRITE YOUR DRIVER CODE HERE
    // PARSE YOUR CLI PARAMETERS
    // THEN ADD THE SPECIFIED FILE TO YOUR FILE SYSTEM
    // UPDATE THE .IMG FILE ON DISK
    cli_t cli = parse_cli(argc, argv);

    FILE *f = fopen(cli.input, "rb");
    if (!f) { perror(cli.input); return 1; }
    if (fseek(f, 0, SEEK_END)) { perror("fseek"); fclose(f); return 1; }
    long imgsz_off = ftell(f);
    if (imgsz_off < 0) { perror("ftell"); fclose(f); return 1; }
    size_t imgsz = (size_t)imgsz_off;
    if (fseek(f, 0, SEEK_SET)) { perror("fseek"); fclose(f); return 1; }

    uint8_t *image = (uint8_t*)malloc(imgsz);
    if (!image) { fclose(f); die("malloc image"); }
    if (fread(image, 1, imgsz, f) != imgsz) { fclose(f); free(image); die("read image failed"); }
    fclose(f);

    if (imgsz < BS) { free(image); die("image too small"); }
    superblock_t *sb = (superblock_t*)image;
    if (sb->magic != 0x4D565346u) { free(image); die("bad magic"); }
    if (sb->block_size != BS) { free(image); die("unexpected block size"); }

    uint8_t *inode_bm = image + sb->inode_bitmap_start * BS;
    uint8_t *data_bm  = image + sb->data_bitmap_start * BS;
    inode_t *inode_tbl = (inode_t*)(image + sb->inode_table_start * BS);
    uint8_t *data_region = image + sb->data_region_start * BS;

    FILE *af = fopen(cli.file, "rb");
    if (!af) { perror(cli.file); free(image); return 1; }
    if (fseek(af, 0, SEEK_END)) { perror("fseek"); fclose(af); free(image); return 1; }
    long addsz_off = ftell(af);
    if (addsz_off < 0) { perror("ftell"); fclose(af); free(image); return 1; }
    size_t addlen = (size_t)addsz_off;
    if (fseek(af, 0, SEEK_SET)) { perror("fseek"); fclose(af); free(image); return 1; }

    uint8_t *buf = (uint8_t*)malloc(addlen ? addlen : 1);
    if (!buf) { fclose(af); free(image); die("malloc"); }
    if (addlen && fread(buf, 1, addlen, af) != addlen) { fclose(af); free(image); free(buf); die("read file failed"); }
    fclose(af);

    if (addlen > DIRECT_MAX * BS) {
        fprintf(stderr, "error: file too large for 12 direct blocks (max %u bytes)\n", DIRECT_MAX * BS);
        free(image); free(buf); return 1;
    }

    uint64_t free_inode_index = UINT64_MAX;
    for (uint64_t i = 0; i < sb->inode_count; i++) {
        if (!test_bit(inode_bm, i)) { free_inode_index = i; break; }
    }
    if (free_inode_index == UINT64_MAX) { fprintf(stderr, "no free inode\n"); free(image); free(buf); return 1; }
    uint32_t ino_no = (uint32_t)(free_inode_index + 1);
    inode_t *ino = &inode_tbl[free_inode_index];
    memset(ino, 0, sizeof(*ino));
    ino->mode = 0x8000;
    ino->links = 1;
    ino->size_bytes = addlen;
    uint64_t now = (uint64_t)time(NULL);
    ino->atime = ino->mtime = ino->ctime = now;
    ino->proj_id = 2;

    size_t rem = addlen, off = 0; int di = 0;
    for (uint64_t dbi = 0; dbi < sb->data_region_blocks && rem > 0 && di < DIRECT_MAX; dbi++) {
        if (!test_bit(data_bm, dbi)) {
            set_bit(data_bm, dbi);
            size_t tocpy = rem > BS ? BS : rem;
            memcpy(data_region + dbi * BS, buf + off, tocpy);
            ino->direct[di] = (uint32_t)(sb->data_region_start + dbi);
            rem -= tocpy; off += tocpy; di++;
        }
    }
    if (rem > 0) { fprintf(stderr, "not enough free data blocks\n"); free(image); free(buf); return 1; }
    inode_crc_finalize(ino);
    set_bit(inode_bm, free_inode_index);

    inode_t *root = &inode_tbl[0];
    uint64_t root_size = root->size_bytes;
    uint64_t last_block_logical = root_size / BS;
    size_t last_off_in_blk = (size_t)(root_size % BS);

    if (last_block_logical >= DIRECT_MAX) { fprintf(stderr, "root out of direct blocks\n"); free(image); free(buf); return 1; }
    if (root->direct[last_block_logical] == 0 && last_off_in_blk == 0) {
        uint64_t new_dbi = UINT64_MAX;
        for (uint64_t dbi = 0; dbi < sb->data_region_blocks; dbi++) if (!test_bit(data_bm, dbi)) { new_dbi = dbi; break; }
        if (new_dbi == UINT64_MAX) { fprintf(stderr, "no free block for root\n"); free(image); free(buf); return 1; }
        set_bit(data_bm, new_dbi);
        root->direct[last_block_logical] = (uint32_t)(sb->data_region_start + new_dbi);
    }

    uint32_t abs_blk = root->direct[last_block_logical];
    uint64_t dbi_of_block = (uint64_t)abs_blk - sb->data_region_start;
    uint8_t *dirblk = data_region + dbi_of_block * BS;

    dirent64_t de;
    memset(&de, 0, sizeof(de));
    de.inode_no = ino_no;
    de.type = 1;
    strncpy(de.name, strrchr(cli.file, '/') ? strrchr(cli.file, '/')+1 : cli.file, sizeof(de.name)-1);
    dirent_checksum_finalize(&de);

    if (last_off_in_blk + sizeof(de) > BS) {
        uint64_t new_dbi = UINT64_MAX;
        for (uint64_t dbi = 0; dbi < sb->data_region_blocks; dbi++) if (!test_bit(data_bm, dbi)) { new_dbi = dbi; break; }
        if (new_dbi == UINT64_MAX) { fprintf(stderr, "no free block for root dir\n"); free(image); free(buf); return 1; }
        set_bit(data_bm, new_dbi);
        last_block_logical++;
        if (last_block_logical >= DIRECT_MAX) { fprintf(stderr, "root dir full\n"); free(image); free(buf); return 1; }
        root->direct[last_block_logical] = (uint32_t)(sb->data_region_start + new_dbi);
        dirblk = data_region + new_dbi * BS;
        last_off_in_blk = 0;
    }

    memcpy(dirblk + last_off_in_blk, &de, sizeof(de));
    root->size_bytes += sizeof(de);
    root->links += 1;
    inode_crc_finalize(root);

    superblock_crc_finalize(sb);

    FILE *out = fopen(cli.output, "wb");
    if (!out) { perror(cli.output); free(image); free(buf); return 1; }
    if (fwrite(image, 1, imgsz, out) != imgsz) { perror("fwrite"); fclose(out); free(image); free(buf); return 1; }
    fclose(out);

    free(image);
    free(buf);
    return 0;
}

