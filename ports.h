#include <avr/pgmspace.h>

#define REMAP_PINS( \
                   as, ai, ad, \
                   bs, bi, bd, \
                   cs, ci, cd, \
                   ds, di, dd, \
                   es, ei, ed, \
                   fs, fi, fd, \
                   gs, gi, gd, \
                   hs, hi, hd )\
            ad = (ad & ~(1<<ai)) | (!!(as & 1<<0) << ai); \
            bd = (bd & ~(1<<bi)) | (!!(bs & 1<<1) << bi); \
            cd = (cd & ~(1<<ci)) | (!!(cs & 1<<2) << ci); \
            dd = (dd & ~(1<<di)) | (!!(ds & 1<<3) << di); \
            ed = (ed & ~(1<<ei)) | (!!(es & 1<<4) << ei); \
            fd = (fd & ~(1<<fi)) | (!!(fs & 1<<5) << fi); \
            gd = (gd & ~(1<<gi)) | (!!(gs & 1<<6) << gi); \
            hd = (hd & ~(1<<hi)) | (!!(hs & 1<<7) << hi);
#define REMAP_PINS_INV( \
                   as, ai, ad, \
                   bs, bi, bd, \
                   cs, ci, cd, \
                   ds, di, dd, \
                   es, ei, ed, \
                   fs, fi, fd, \
                   gs, gi, gd, \
                   hs, hi, hd )\
            ad = (ad & ~(1<<0)) | (!!(as & 1<<ai) << 0); \
            bd = (bd & ~(1<<1)) | (!!(bs & 1<<bi) << 1); \
            cd = (cd & ~(1<<2)) | (!!(cs & 1<<ci) << 2); \
            dd = (dd & ~(1<<3)) | (!!(ds & 1<<di) << 3); \
            ed = (ed & ~(1<<4)) | (!!(es & 1<<ei) << 4); \
            fd = (fd & ~(1<<5)) | (!!(fs & 1<<fi) << 5); \
            gd = (gd & ~(1<<6)) | (!!(gs & 1<<gi) << 6); \
            hd = (hd & ~(1<<7)) | (!!(hs & 1<<hi) << 7);

void port0_init(uint8_t src);
void port0_write(uint8_t src);
uint8_t port0_read(void);
void port1_init(uint8_t src);
void port1_write(uint8_t src);
uint8_t port1_read(void);
void port2_init(uint8_t src);
void port2_write(uint8_t src);
uint8_t port2_read(void);
void rst_init(void);
uint8_t rst_read(void);
void set_selector(uint8_t selector);
uint8_t read_keys(void);
uint8_t scan_line(uint8_t selector);
