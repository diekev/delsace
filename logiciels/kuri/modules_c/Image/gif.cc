/* Original code released in the public domain by
 * https://github.com/lecram/gifdec
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Modifications Copyright (C) 2025 Kévin Dietrich. */

#include "gif.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#    include <io.h>
#else
#    include <unistd.h>
#endif

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

typedef struct Entry {
    uint16_t length;
    uint16_t prefix;
    uint8_t suffix;
} Entry;

typedef struct Table {
    int bulk;
    int nentries;
    Entry *entries;
} Table;

class ChargeGIF {
  public:
    virtual ~ChargeGIF() = default;

    virtual ssize_t read(void *buf, size_t nbytes) = 0;
    virtual __off_t lseek(__off_t offset, int whence) = 0;

    uint16_t read_num()
    {
        uint8_t bytes[2];
        this->read(bytes, 2);
        return bytes[0] + (((uint16_t)bytes[1]) << 8);
    }
};

class ChargeGIFFIchier : public ChargeGIF {
    int m_fd;

  public:
    ChargeGIFFIchier(int fd) : m_fd(fd)
    {
#ifdef _WIN32
        setmode(fd, O_BINARY);
#endif
    }

    ~ChargeGIFFIchier() override
    {
        close(m_fd);
    }

  protected:
    ssize_t read(void *buf, size_t nbytes) override
    {
        return ::read(m_fd, buf, nbytes);
    }

    __off_t lseek(__off_t offset, int whence) override
    {
        return ::lseek(m_fd, offset, whence);
    }
};

class ChargeGIFMémoire : public ChargeGIF {
    const void *m_buf = nullptr;
    uint64_t m_size = 0;
    size_t m_curseur = 0;

  public:
    ChargeGIFMémoire(const void *buf, uint64_t size) : m_buf(buf), m_size(size)
    {
    }

    ssize_t read(void *buf, size_t nbytes) override
    {
        if (m_curseur + nbytes > m_size) {
            return 0;
        }

        memcpy(buf, static_cast<const char *>(m_buf) + m_curseur, nbytes);
        m_curseur += nbytes;
        return nbytes;
    }

    __off_t lseek(__off_t offset, int whence) override
    {
        switch (whence) {
            case SEEK_SET:
            {
                m_curseur = offset;
                break;
            }
            case SEEK_CUR:
            {
                m_curseur += offset;
                break;
            }
            case SEEK_END:
            {
                m_curseur = m_size - 1 - offset;
                break;
            }
        }

        return m_curseur;
    }
};

static gd_GIF *gd_open_gif(ChargeGIF *charge_gif)
{
    uint8_t sigver[3];

    /* Header */
    charge_gif->read(sigver, 3);
    if (memcmp(sigver, "GIF", 3) != 0) {
        fprintf(stderr, "[GIF] invalid signature\n");
        return nullptr;
    }
    /* Version */
    charge_gif->read(sigver, 3);
    /* À FAIRE : le code fut écris pour 89a, ajout de 87a ici pour charger les fichiers, mais il
     * faudra comprendre la différence et modifier le code pour prendre en compte la bonne version.
     */
    if (memcmp(sigver, "87a", 3) != 0 && memcmp(sigver, "89a", 3) != 0) {
        fprintf(stderr,
                "[GIF] invalid or unsupported version, got %c%c%c\n",
                sigver[0],
                sigver[1],
                sigver[2]);
        return nullptr;
    }
    /* Width x Height */
    uint16_t width = charge_gif->read_num();
    uint16_t height = charge_gif->read_num();
    /* FDSZ */
    uint8_t fdsz;
    charge_gif->read(&fdsz, 1);
    /* Presence of GCT */
    if (!(fdsz & 0x80)) {
        fprintf(stderr, "[GIF] no global color table\n");
        return nullptr;
    }
    /* Color Space's Depth */
    uint16_t depth = ((fdsz >> 4) & 7) + 1;
    /* Ignore Sort Flag. */
    /* GCT Size */
    int gct_sz = 1 << ((fdsz & 0x07) + 1);
    /* Background Color Index */
    uint8_t bgidx;
    charge_gif->read(&bgidx, 1);
    /* Aspect Ratio */
    uint8_t aspect;
    charge_gif->read(&aspect, 1);
    /* Create gd_GIF Structure. */
    gd_GIF *gif = static_cast<gd_GIF *>(calloc(1, sizeof(*gif)));
    if (!gif)
        return nullptr;
    gif->charge_gif = charge_gif;
    gif->width = width;
    gif->height = height;
    gif->depth = depth;
    /* Read GCT */
    gif->gct.size = gct_sz;
    charge_gif->read(gif->gct.colors, 3 * gif->gct.size);
    gif->palette = &gif->gct;
    gif->bgindex = bgidx;
    gif->frame = static_cast<unsigned char *>(calloc(4, width * height));
    if (!gif->frame) {
        free(gif);
        return nullptr;
    }
    gif->canvas = &gif->frame[width * height];
    if (gif->bgindex)
        memset(gif->frame, gif->bgindex, gif->width * gif->height);
    uint8_t *bgcolor = &gif->palette->colors[gif->bgindex * 3];
    if (bgcolor[0] || bgcolor[1] || bgcolor[2])
        for (int i = 0; i < gif->width * gif->height; i++)
            memcpy(&gif->canvas[i * 3], bgcolor, 3);
    gif->anim_start = charge_gif->lseek(0, SEEK_CUR);
    return gif;
}

gd_GIF *gd_open_gif_from_file(const char *fname)
{
    int fd = open(fname, O_RDONLY);
    if (fd == -1) {
        return nullptr;
    }

    auto charge_gif = new ChargeGIFFIchier(fd);
    auto résultat = gd_open_gif(charge_gif);
    if (résultat == nullptr) {
        delete charge_gif;
    }
    return résultat;
}

gd_GIF *gd_open_gif_from_memory(const void *buf, uint64_t size)
{
    auto charge_gif = new ChargeGIFMémoire(buf, size);
    auto résultat = gd_open_gif(charge_gif);
    if (résultat == nullptr) {
        delete charge_gif;
    }
    return résultat;
}

static void discard_sub_blocks(gd_GIF *gif)
{
    uint8_t size;

    do {
        gif->charge_gif->read(&size, 1);
        gif->charge_gif->lseek(size, SEEK_CUR);
    } while (size);
}

static void read_plain_text_ext(gd_GIF *gif)
{
    if (gif->plain_text) {
        uint16_t tx, ty, tw, th;
        uint8_t cw, ch, fg, bg;
        off_t sub_block;
        gif->charge_gif->lseek(1, SEEK_CUR); /* block size = 12 */
        tx = gif->charge_gif->read_num();
        ty = gif->charge_gif->read_num();
        tw = gif->charge_gif->read_num();
        th = gif->charge_gif->read_num();
        gif->charge_gif->read(&cw, 1);
        gif->charge_gif->read(&ch, 1);
        gif->charge_gif->read(&fg, 1);
        gif->charge_gif->read(&bg, 1);
        sub_block = gif->charge_gif->lseek(0, SEEK_CUR);
        gif->plain_text(gif, tx, ty, tw, th, cw, ch, fg, bg);
        gif->charge_gif->lseek(sub_block, SEEK_SET);
    }
    else {
        /* Discard plain text metadata. */
        gif->charge_gif->lseek(13, SEEK_CUR);
    }
    /* Discard plain text sub-blocks. */
    discard_sub_blocks(gif);
}

static void read_graphic_control_ext(gd_GIF *gif)
{
    uint8_t rdit;

    /* Discard block size (always 0x04). */
    gif->charge_gif->lseek(1, SEEK_CUR);
    gif->charge_gif->read(&rdit, 1);
    gif->gce.disposal = (rdit >> 2) & 3;
    gif->gce.input = rdit & 2;
    gif->gce.transparency = rdit & 1;
    gif->gce.delay = gif->charge_gif->read_num();
    gif->charge_gif->read(&gif->gce.tindex, 1);
    /* Skip block terminator. */
    gif->charge_gif->lseek(1, SEEK_CUR);
}

static void read_comment_ext(gd_GIF *gif)
{
    if (gif->comment) {
        off_t sub_block = gif->charge_gif->lseek(0, SEEK_CUR);
        gif->comment(gif);
        gif->charge_gif->lseek(sub_block, SEEK_SET);
    }
    /* Discard comment sub-blocks. */
    discard_sub_blocks(gif);
}

static void read_application_ext(gd_GIF *gif)
{
    char app_id[8];
    char app_auth_code[3];

    /* Discard block size (always 0x0B). */
    gif->charge_gif->lseek(1, SEEK_CUR);
    /* Application Identifier. */
    gif->charge_gif->read(app_id, 8);
    /* Application Authentication Code. */
    gif->charge_gif->read(app_auth_code, 3);
    if (!strncmp(app_id, "NETSCAPE", sizeof(app_id))) {
        /* Discard block size (0x03) and constant byte (0x01). */
        gif->charge_gif->lseek(2, SEEK_CUR);
        gif->loop_count = gif->charge_gif->read_num();
        /* Skip block terminator. */
        gif->charge_gif->lseek(1, SEEK_CUR);
    }
    else if (gif->application) {
        off_t sub_block = gif->charge_gif->lseek(0, SEEK_CUR);
        gif->application(gif, app_id, app_auth_code);
        gif->charge_gif->lseek(sub_block, SEEK_SET);
        discard_sub_blocks(gif);
    }
    else {
        discard_sub_blocks(gif);
    }
}

static void read_ext(gd_GIF *gif)
{
    uint8_t label;

    gif->charge_gif->read(&label, 1);
    switch (label) {
        case 0x01:
            read_plain_text_ext(gif);
            break;
        case 0xF9:
            read_graphic_control_ext(gif);
            break;
        case 0xFE:
            read_comment_ext(gif);
            break;
        case 0xFF:
            read_application_ext(gif);
            break;
        default:
            fprintf(stderr, "unknown extension: %02X\n", label);
    }
}

static Table *new_table(int key_size)
{
    int key;
    int init_bulk = MAX(1 << (key_size + 1), 0x100);
    Table *table = static_cast<Table *>(malloc(sizeof(*table) + sizeof(Entry) * init_bulk));
    if (table) {
        table->bulk = init_bulk;
        table->nentries = (1 << key_size) + 2;
        table->entries = (Entry *)&table[1];
        for (key = 0; key < (1 << key_size); key++)
            table->entries[key] = Entry{1, 0xFFF, uint8_t(key)};
    }
    return table;
}

/* Add table entry. Return value:
 *  0 on success
 *  +1 if key size must be incremented after this addition
 *  -1 if could not realloc table */
static int add_entry(Table **tablep, uint16_t length, uint16_t prefix, uint8_t suffix)
{
    Table *table = *tablep;
    if (table->nentries == table->bulk) {
        table->bulk *= 2;
        table = static_cast<Table *>(realloc(table, sizeof(*table) + sizeof(Entry) * table->bulk));
        if (!table)
            return -1;
        table->entries = (Entry *)&table[1];
        *tablep = table;
    }
    table->entries[table->nentries] = (Entry){length, prefix, suffix};
    table->nentries++;
    if ((table->nentries & (table->nentries - 1)) == 0)
        return 1;
    return 0;
}

static uint16_t get_key(gd_GIF *gif, int key_size, uint8_t *sub_len, uint8_t *shift, uint8_t *byte)
{
    int bits_read;
    int rpad;
    int frag_size;
    uint16_t key;

    key = 0;
    for (bits_read = 0; bits_read < key_size; bits_read += frag_size) {
        rpad = (*shift + bits_read) % 8;
        if (rpad == 0) {
            /* Update byte. */
            if (*sub_len == 0) {
                gif->charge_gif->read(sub_len, 1); /* Must be nonzero! */
                if (*sub_len == 0)
                    return 0x1000;
            }
            gif->charge_gif->read(byte, 1);
            (*sub_len)--;
        }
        frag_size = MIN(key_size - bits_read, 8 - rpad);
        key |= ((uint16_t)((*byte) >> rpad)) << bits_read;
    }
    /* Clear extra bits to the left. */
    key &= (1 << key_size) - 1;
    *shift = (*shift + key_size) % 8;
    return key;
}

/* Compute output index of y-th input line, in frame of height h. */
static int interlaced_line_index(int h, int y)
{
    int p; /* number of lines in current pass */

    p = (h - 1) / 8 + 1;
    if (y < p) /* pass 1 */
        return y * 8;
    y -= p;
    p = (h - 5) / 8 + 1;
    if (y < p) /* pass 2 */
        return y * 8 + 4;
    y -= p;
    p = (h - 3) / 4 + 1;
    if (y < p) /* pass 3 */
        return y * 4 + 2;
    y -= p;
    /* pass 4 */
    return y * 2 + 1;
}

/* Decompress image pixels.
 * Return 0 on success or -1 on out-of-memory (w.r.t. LZW code table). */
static int read_image_data(gd_GIF *gif, int interlace)
{
    uint8_t sub_len, shift, byte;
    int init_key_size, key_size, table_is_full;
    int frm_off, frm_size, str_len, i, p, x, y;
    uint16_t key, clear, stop;
    int ret;
    Table *table;
    Entry entry;
    off_t start, end;

    gif->charge_gif->read(&byte, 1);
    key_size = (int)byte;
    if (key_size < 2 || key_size > 8)
        return -1;

    start = gif->charge_gif->lseek(0, SEEK_CUR);
    discard_sub_blocks(gif);
    end = gif->charge_gif->lseek(0, SEEK_CUR);
    gif->charge_gif->lseek(start, SEEK_SET);
    clear = 1 << key_size;
    stop = clear + 1;
    table = new_table(key_size);
    key_size++;
    init_key_size = key_size;
    sub_len = shift = 0;
    key = get_key(gif, key_size, &sub_len, &shift, &byte); /* clear code */
    frm_off = 0;
    ret = 0;
    frm_size = gif->fw * gif->fh;
    while (frm_off < frm_size) {
        if (key == clear) {
            key_size = init_key_size;
            table->nentries = (1 << (key_size - 1)) + 2;
            table_is_full = 0;
        }
        else if (!table_is_full) {
            ret = add_entry(&table, str_len + 1, key, entry.suffix);
            if (ret == -1) {
                free(table);
                return -1;
            }
            if (table->nentries == 0x1000) {
                ret = 0;
                table_is_full = 1;
            }
        }
        key = get_key(gif, key_size, &sub_len, &shift, &byte);
        if (key == clear)
            continue;
        if (key == stop || key == 0x1000)
            break;
        if (ret == 1)
            key_size++;
        entry = table->entries[key];
        str_len = entry.length;
        for (i = 0; i < str_len; i++) {
            p = frm_off + entry.length - 1;
            x = p % gif->fw;
            y = p / gif->fw;
            if (interlace)
                y = interlaced_line_index((int)gif->fh, y);
            gif->frame[(gif->fy + y) * gif->width + gif->fx + x] = entry.suffix;
            if (entry.prefix == 0xFFF)
                break;
            else
                entry = table->entries[entry.prefix];
        }
        frm_off += str_len;
        if (key < table->nentries - 1 && !table_is_full)
            table->entries[table->nentries - 1].suffix = entry.suffix;
    }
    free(table);
    if (key == stop)
        gif->charge_gif->read(&sub_len, 1); /* Must be zero! */
    gif->charge_gif->lseek(end, SEEK_SET);
    return 0;
}

/* Read image.
 * Return 0 on success or -1 on out-of-memory (w.r.t. LZW code table). */
static int read_image(gd_GIF *gif)
{
    uint8_t fisrz;
    int interlace;

    /* Image Descriptor. */
    gif->fx = gif->charge_gif->read_num();
    gif->fy = gif->charge_gif->read_num();

    if (gif->fx >= gif->width || gif->fy >= gif->height)
        return -1;

    gif->fw = gif->charge_gif->read_num();
    gif->fh = gif->charge_gif->read_num();

    gif->fw = MIN(gif->fw, gif->width - gif->fx);
    gif->fh = MIN(gif->fh, gif->height - gif->fy);

    gif->charge_gif->read(&fisrz, 1);
    interlace = fisrz & 0x40;
    /* Ignore Sort Flag. */
    /* Local Color Table? */
    if (fisrz & 0x80) {
        /* Read LCT */
        gif->lct.size = 1 << ((fisrz & 0x07) + 1);
        gif->charge_gif->read(gif->lct.colors, 3 * gif->lct.size);
        gif->palette = &gif->lct;
    }
    else
        gif->palette = &gif->gct;
    /* Image Data. */
    return read_image_data(gif, interlace);
}

static void render_frame_rect(gd_GIF *gif, uint8_t *buffer)
{
    int i, j, k;
    uint8_t index, *color;
    i = gif->fy * gif->width + gif->fx;
    for (j = 0; j < gif->fh; j++) {
        for (k = 0; k < gif->fw; k++) {
            index = gif->frame[(gif->fy + j) * gif->width + gif->fx + k];
            color = &gif->palette->colors[index * 3];
            if (!gif->gce.transparency || index != gif->gce.tindex)
                memcpy(&buffer[(i + k) * 3], color, 3);
        }
        i += gif->width;
    }
}

static void dispose(gd_GIF *gif)
{
    int i, j, k;
    uint8_t *bgcolor;
    switch (gif->gce.disposal) {
        case 2: /* Restore to background color. */
            bgcolor = &gif->palette->colors[gif->bgindex * 3];
            i = gif->fy * gif->width + gif->fx;
            for (j = 0; j < gif->fh; j++) {
                for (k = 0; k < gif->fw; k++)
                    memcpy(&gif->canvas[(i + k) * 3], bgcolor, 3);
                i += gif->width;
            }
            break;
        case 3: /* Restore to previous, i.e., don't update canvas.*/
            break;
        default:
            /* Add frame non-transparent pixels to canvas. */
            render_frame_rect(gif, gif->canvas);
    }
}

/* Return 1 if got a frame; 0 if got GIF trailer; -1 if error. */
int gd_get_frame(gd_GIF *gif)
{
    char sep;

    dispose(gif);
    gif->charge_gif->read(&sep, 1);
    while (sep != ',') {
        if (sep == ';')
            return 0;
        if (sep == '!')
            read_ext(gif);
        else
            return -1;
        gif->charge_gif->read(&sep, 1);
    }
    if (read_image(gif) == -1)
        return -1;
    return 1;
}

void gd_render_frame(gd_GIF *gif, uint8_t *buffer)
{
    memcpy(buffer, gif->canvas, gif->width * gif->height * 3);
    render_frame_rect(gif, buffer);
}

int gd_is_bgcolor(gd_GIF *gif, uint8_t color[3])
{
    return !memcmp(&gif->palette->colors[gif->bgindex * 3], color, 3);
}

void gd_rewind(gd_GIF *gif)
{
    gif->charge_gif->lseek(gif->anim_start, SEEK_SET);
}

void gd_close_gif(gd_GIF *gif)
{
    delete gif->charge_gif;
    free(gif->frame);
    free(gif);
}
