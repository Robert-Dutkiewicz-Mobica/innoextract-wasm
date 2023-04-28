#ifndef NONZIP_H
#define NONZIP_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define NONZIP_LF_SIGNATURE   (0x04034b50U)
#define NONZIP_CF_SIGNATURE   (0x02014b50U)
#define NONZIP_END_SIGNATURE  (0x06054b50U)
#define NONZIP_EF_ZIP64_SIG   (0x0001U)
#define NONZIP_ZIP64_END_SIG  (0x06064b50U)
#define NONZIP_ZIP64_LOC_SIG  (0x07064b50U)
#define NONZIP_DATA_DESC_SIG  (0x08074b50U)
#define NONZIP_VERMADE_DEF    (63U) // Version 6.3 of document
#define NONZIP_VERNEED_DEF    (45U) // Version 4.5 required for ZIP64

// Bit 11: Enable UTF filenames and comments, bit 3: lengths and CRC only correct in central header
#define NONZIP_FLAGS_DEF      (/*(1<<11) | */(1<<3))
#define NONZIP_COMP_DEF       (0U) // No compression
#define NONZIP_DISK_DEF       (0U) // only "disk 0" used
#define NONZIP_ATTRS_DEF      (0U) // attributes might be set to 0

#define NONZIP_STATUS_IDLE    (0)
#define NONZIP_STATUS_READY   (1)
#define NONZIP_ERR_NOTREADY   (-1)
const char nonzip_comment[] = "Created with nonzip by Michał Stoń <michal.ston@mobica.com>";

#define IFFITS32(x) (x < 0xFFFFFFFF ? x : 0xFFFFFFFF) // return var if fits in 32b, otherwise 0xFFFFFFFF
#define IFFITS16(x) (x < 0xFFFF ? x : 0xFFFF) // return var if fits in 32b, otherwise 0xFFFFFFFF

struct dostime{
    uint16_t time;
    uint16_t date;
};

// Struct for internal use to keep all fire parameters used by CFH and LFH
struct nonzip_file {
    uint16_t zf_modt;   // last mod file time
    uint16_t zf_modd;   // last mod file date
    uint32_t zf_crc;    // crc-32
    uint64_t zf_csize;  // compressed size
    uint64_t zf_usize;  // uncompressed size
    uint16_t zf_fnlen;  // file name length
    // uint16_t zf_eflen;  // extra field length
    // uint16_t zf_fclen;  // file comment length
    uint16_t zf_dns;    // disk number start
    // uint16_t zf_attri;  // internal file attributes
    // uint32_t zf_attre;  // external file attributes
    uint64_t zf_offset;  // relative offset of local header
    char     *zf_name;
    char     *zf_data;
};

#pragma pack(push, 1)
// Local file header
struct nonzip_lf {
    uint32_t lf_sig;    // local file header signature
    uint16_t lf_vere;   // version needed to extract
    uint16_t lf_flags;  // general purpose bit flag
    uint16_t lf_comp;   // compression method
    uint16_t lf_modt;   // time in MS-DOS format
    uint16_t lf_modd;   // date, ditto
    uint32_t lf_crc;    // CRC32, magic=0xdebb20e3
    uint32_t lf_csize;  // compressed size
    uint32_t lf_usize;  // uncompressed size
    uint16_t lf_fnlen;  // file name length
    uint16_t lf_eflen;  // extra field length
    // filename
    // extra field
    // actual data
};

// Central file header
struct nonzip_cf {
    uint32_t cf_sig;    // central file header signature
    uint16_t cf_verm;   // version made by
    uint16_t cf_vere;   // version needed to extract
    uint16_t cf_flags;  // general purpose bit flag
    uint16_t cf_comp;   // compression method
    uint16_t cf_modt;   // last mod file time
    uint16_t cf_modd;   // last mod file date
    uint32_t cf_crc;    // crc-32
    uint32_t cf_csize;  // compressed size
    uint32_t cf_usize;  // uncompressed size
    uint16_t cf_fnlen;  // file name length
    uint16_t cf_eflen;  // extra field length
    uint16_t cf_fclen;  // file comment length
    uint16_t cf_dns;    // disk number start
    uint16_t cf_attri;  // internal file attributes
    uint32_t cf_attre;  // external file attributes
    uint32_t cf_offset;  // relative offset of local header
    // filename
    // extra field
    // file comment
};

struct nonzip_end {
    uint32_t en_sig;    // end of central dir signature
    uint16_t en_dnum;   // number of this disk
    uint16_t en_cdnum;  // number of the disk with the
                        // start of the central directory
    uint16_t en_ennum;  // total number of entries in the
                        // central directory on this disk
    uint16_t en_cdennum;// total number of entries in
                        // the central directory
    uint32_t en_cdsize; // size of the central directory
    uint32_t en_cdoff;  // offset of start of central
                        // directory with respect to
                        // the starting disk number
    uint16_t en_clen;   // .ZIP file comment length = NONZIP_ZIPCOMMENT_LEN
    char en_comment[sizeof(nonzip_comment)-1]; // comment
};

// ZIP64 
struct nonzip_z64ef {
    uint16_t ef_sig;    // extra field signature
    uint16_t ef_size;   // extra field size
    uint64_t ef_usize;  // uncompressed size
    uint64_t ef_csize;  // compressed size
    uint64_t ef_offset; // relative file header offset
};

struct nonzip_z64end {
    uint32_t en_sig;    // end of central dir signature
    uint64_t en_size;   // size of this directory // size of this struct - 12(sig+size)
    uint16_t en_verm;   // version made by
    uint16_t en_vere;   // version needed to extract
    uint32_t en_dnum;   // number of this disk
    uint32_t en_cdnum;  // number of the disk with the
                        // start of the central directory
    uint64_t en_ennum;  // total number of entries in the
                        // central directory on this disk
    uint64_t en_cdennum;// total number of entries in
                        // the central directory
    uint64_t en_cdsize; // size of the central directory
    uint64_t en_cdoff;  // offset of start of central
                        // directory with respect to
                        // the starting disk number
};

struct nonzip_z64loc {
    uint32_t loc_sig;   // extra field signature
    uint32_t loc_endnum;// number of the disk with the Z64 CD end record
    uint64_t loc_off;   // offset of start of Z64 CD end record
    uint32_t loc_numd;  // total number of disks    
};

struct nonzip_dd {
    uint32_t dd_sig;    // extra field signature
    uint32_t dd_crc;    // crc32
    uint64_t dd_usize;  // uncompressed size
    uint64_t dd_csize;  // compressed size
};


#pragma pack(pop)

class Nonzip {
private:
    int status;
    uint64_t numfiles;
    uint64_t numfalloc;
    uint64_t offset;
    struct nonzip_file **files;
    struct dostime dost;
    void writeDD();
public:
    Nonzip(const char *path);
    int addFile(const char *name, const void *data, size_t dlen, uint32_t *zipindex);
    int appendFile(const void *data, size_t dlen);
    int close();
    int getStatus();
    void setTime(uint32_t index, time_t t);
};

#endif /* NONZIP_H */
