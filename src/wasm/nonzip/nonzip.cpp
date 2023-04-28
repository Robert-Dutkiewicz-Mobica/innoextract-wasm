#include "nonzip.h"
#include <inttypes.h>
#include "crc32.h"

#define NONZIP_PLATFORM

#ifdef NONZIP_PLATFORM
#include "wasm/emjs.h"
#endif

static inline void filetolf(struct nonzip_lf *lf, struct nonzip_file *f) {
    lf->lf_sig   = NONZIP_LF_SIGNATURE;
    lf->lf_vere = NONZIP_VERNEED_DEF;
    lf->lf_flags = NONZIP_FLAGS_DEF;
    lf->lf_comp = NONZIP_COMP_DEF;
    lf->lf_modt  = f->zf_modt;
    lf->lf_modd  = f->zf_modd;
    lf->lf_csize = 0;
    lf->lf_usize = 0;
    // CRC, csize and usize are ignored in local header
    lf->lf_fnlen = f->zf_fnlen;
    lf->lf_eflen = 0;
    // lf->lf_eflen = sizeof(struct nonzip_z64ef);
    // Unassigned fields are ignored anyway 
}

static inline void filetocf(struct nonzip_cf *cf, struct nonzip_file *f) {
    cf->cf_sig    = NONZIP_CF_SIGNATURE;
    cf->cf_verm   = NONZIP_VERMADE_DEF;
    cf->cf_vere   = NONZIP_VERNEED_DEF;
    cf->cf_flags  = NONZIP_FLAGS_DEF;
    cf->cf_comp   = NONZIP_COMP_DEF;
    cf->cf_modt   = f->zf_modt;
    cf->cf_modd   = f->zf_modd;
    cf->cf_crc    = f->zf_crc;
    cf->cf_csize  = IFFITS32(f->zf_csize);
    cf->cf_usize  = IFFITS32(f->zf_usize);
    cf->cf_fnlen  = f->zf_fnlen;
    cf->cf_dns    = 0;
    cf->cf_offset = IFFITS32(f->zf_offset);
    cf->cf_eflen  = sizeof(struct nonzip_z64ef);
    // Unassigned fields are ignored anyway 
}

static inline void filetoz64ef(struct nonzip_z64ef *ef, struct nonzip_file *f) {
    ef->ef_sig    = NONZIP_EF_ZIP64_SIG;
    ef->ef_size   = sizeof(struct nonzip_z64ef) - 4;
    ef->ef_csize  = f->zf_csize;
    ef->ef_usize  = f->zf_usize;
    ef->ef_offset = f->zf_offset;
}

static inline struct dostime timetodos(time_t tt) {
    struct tm l = *localtime(&tt);
    return (struct dostime){l.tm_hour << 11 | l.tm_min << 5 | l.tm_sec >> 1, (l.tm_year - 80) << 9 | (l.tm_mon + 1) << 5 | l.tm_mday};
}

Nonzip::Nonzip(const char *path) {
    emjs::open(path, "wb");
    dost = timetodos(time(0));
    status = NONZIP_STATUS_READY;
}

void Nonzip::writeDD()
{
    if(numfiles > 0){
        struct nonzip_dd dd;
        dd.dd_sig = NONZIP_DATA_DESC_SIG;
        dd.dd_crc = files[numfiles-1]->zf_crc;
        dd.dd_csize = files[numfiles-1]->zf_csize;
        dd.dd_usize = files[numfiles-1]->zf_usize;
        offset += emjs::write(&dd, sizeof(dd), 1);
    }
}

int Nonzip::addFile(const char *name, const void *data, size_t dlen, uint32_t *zipindex) {
    if (status != NONZIP_STATUS_READY) return NONZIP_ERR_NOTREADY;

    writeDD(); // Write last file's DD

    // Alloc space for lfhs
    if (numfalloc < numfiles + 1) {
        numfalloc = numfalloc ? numfalloc * 3 / 2 : 8;
        files = (struct nonzip_file **) realloc(files, numfalloc * sizeof(struct nonzip_file *));
    }

    int i = (numfiles++);
    int nlen = strlen(name);
    struct dostime dt = dost;
    files[i] = (struct nonzip_file *)calloc(1, sizeof(struct nonzip_file));

    files[i]->zf_crc = crc32(~0, (uint8_t*) data, dlen);
    files[i]->zf_csize = dlen;
    files[i]->zf_usize = dlen;
    files[i]->zf_fnlen = nlen;
    files[i]->zf_offset = offset;

    files[i]->zf_name = (char *)malloc(nlen);
    memcpy(files[i]->zf_name, name, nlen);

    struct nonzip_lf lf = {0, };
    // struct nonzip_z64ef ef = {0, };
    // ef.ef_sig = NONZIP_EF_ZIP64_SIG;
    // ef.ef_size = sizeof(ef) - 4;

    filetolf(&lf, files[i]);
    emjs::write(&lf, sizeof(lf), 1);   // Write header
    emjs::write(name, 1, nlen);        // Write filename
    // emjs::write(&ef, sizeof(ef), 1);  // write ZIP64 extra field
    dlen = emjs::write(data, 1, dlen); // Write file data

    offset += sizeof(lf) + nlen + dlen;
    // offset += sizeof(lf) + nlen + sizeof(ef) + dlen;

    printf("writing file %s: len=%" "lu" ", offset=%" PRIu64 "\n", name, dlen, files[i]->zf_offset);
    if(zipindex != NULL)
        *zipindex = i;

    return dlen;
}

int Nonzip::appendFile(const void *data, size_t dlen) {
    if (status != NONZIP_STATUS_READY)
        return NONZIP_ERR_NOTREADY;

    long i = numfiles - 1; // Append to most recently added file
    if (i<0)
        return NONZIP_ERR_NOTREADY;

    files[i]->zf_crc = crc32(~files[i]->zf_crc, (uint8_t*) data, dlen);
    files[i]->zf_csize += dlen;
    files[i]->zf_usize += dlen;
    // local file header will not be not updated.
    // valid crc, csize and usize will be only written to the central header for easier appending
    dlen = emjs::write(data, 1, dlen); // Write file data
    offset += dlen;

    // printf("Appending file %s: len=%u(+%u), offset=%u\n", files[i]->zf_name, files[i]->zf_csize , dlen, files[i]->zf_offset);

    return dlen;
}

int Nonzip::close() {
    if (status != NONZIP_STATUS_READY) return NONZIP_ERR_NOTREADY;

    writeDD(); // Write last file's DD

    int i;
    struct nonzip_cf cf = {0, };
    struct nonzip_z64ef ef = {0, };
    uint64_t cdsize = 0;
    uint64_t cdoffset = offset;
    for (i = 0; i < numfiles; i++) {
        filetocf(&cf, files[i]);
        filetoz64ef(&ef, files[i]);
        cdsize += emjs::write(&cf, sizeof(cf), 1);  // write central file header
        cdsize += emjs::write(files[i]->zf_name, 1, files[i]->zf_fnlen);  // write filename
        cdsize += emjs::write(&ef, sizeof(ef), 1);  // write ZIP64 extra field
        free(files[i]->zf_name);
        free(files[i]);
    }
    free(files);

    struct nonzip_z64end en64 = {0, };
    en64.en_sig    = NONZIP_ZIP64_END_SIG;
    en64.en_size   = sizeof(struct nonzip_z64end) - 12; // struct size - sig/size fields
    en64.en_verm   = NONZIP_VERMADE_DEF;
    en64.en_vere   = NONZIP_VERNEED_DEF;
    en64.en_ennum = numfiles;
    en64.en_cdennum = numfiles;
    en64.en_cdoff = cdoffset;
    en64.en_cdsize = cdsize;
    emjs::write(&en64, sizeof(en64), 1);
    
    struct nonzip_z64loc loc = {0, };
    loc.loc_sig = NONZIP_ZIP64_LOC_SIG;
    loc.loc_off = cdoffset + cdsize;
    emjs::write(&loc, sizeof(loc), 1);

    struct nonzip_end end = {0, };
    // memset(&end, 0xFF, sizeof(end));
    end.en_sig = NONZIP_END_SIGNATURE;
    end.en_ennum = IFFITS16(numfiles);
    end.en_cdennum = IFFITS16(numfiles);
    end.en_cdsize = IFFITS32(cdsize);
    end.en_cdoff = ~0; // IFFITS32(cdoffset);
    end.en_clen = sizeof(nonzip_comment) - 1;
    memcpy(end.en_comment, nonzip_comment, sizeof(nonzip_comment) - 1);
    emjs::write(&end, sizeof(end), 1);

    emjs::write(NULL, 0, 0);
    emjs::close();

    status = NONZIP_STATUS_IDLE;
    return numfiles;
}

void Nonzip::setTime(uint32_t index, time_t t)
{
    if(index < numfiles){
        struct dostime dt = timetodos(t);
        files[index]->zf_modd = dost.date;
        files[index]->zf_modt = dost.time;
    }
    else {
        printf("trying to set time for non-existent file, index=%u, numfiles=%llu\n", index, numfiles);
    }
}

int Nonzip::getStatus()
{
    return status;
}
