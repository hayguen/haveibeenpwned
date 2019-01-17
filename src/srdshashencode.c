/*
 * srdshashencode (sorted raw data set hash encode)
 *
 * Copyright 2017 Hayati Ayguen.  Distributed under the terms
 * of the GNU General Public License (GPL)
 *
 * srdshashencode is to prepare for better compression of sorted hashcodes
 *
 * Usage: see below at usage()
 *
 * Author:  Hayati Ayguen
 */


#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef __SSE2__
  #pragma message "__SSE2__ is defined"
  #define SSE2_AVAILABLE  1
#elif defined(_MSC_VER)
  #pragma message "_MSC_VER is defined"
  #if (defined(_M_AMD64) || defined(_M_X64))
    //SSE2 x64
    #define SSE2_AVAILABLE  1
  #elif _M_IX86_FP == 2
    //SSE2 x32
    #define SSE2_AVAILABLE  1
  #endif
#endif

#ifdef SSE2_AVAILABLE
#include <x86intrin.h>
#endif

#if 0
#undef SSE2_AVAILABLE       /* SSE2 speeds up, approx. by factor 2 */
#endif
#define USE_64BIT_ARITH 1   /* on my i5 cpu this speeds up - for blockLen 20 */

static int blockSize = -1;

#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")

#if defined(SSE2_AVAILABLE)
#pragma message "compiling encode() for SSE2"
static inline void encode( __m128i * c, const __m128i * a, const __m128i * b, size_t uBlockLen ) {
  size_t u;
  for ( u = 0; u < uBlockLen; ++u ) {
    c[u] = _mm_sub_epi8( a[u], b[u] );
  }
}
#elif USE_64BIT_ARITH
#pragma message "compiling encode() for 64 Bit arithmetic"
static inline void encode( uint64_t * c, const uint64_t * a, const uint64_t * b, size_t uBlockLen ) {
  size_t u;
  for ( u = 0; u < uBlockLen; ++u ) {
    c[u] =  ( ( ( a[u]  | UINT64_C(0xFF00FF00FF00FF00) )
               -( b[u]  & UINT64_C(0x00FF00FF00FF00FF) )
              )         & UINT64_C(0x00FF00FF00FF00FF)
            )
          | ( ( ( a[u]  | UINT64_C(0x00FF00FF00FF00FF) )
               -( b[u]  & UINT64_C(0xFF00FF00FF00FF00) )
              )         & UINT64_C(0xFF00FF00FF00FF00)
            );
  }
}
#else
#pragma message "compiling encode() with fallback options"
static inline void encode( unsigned char * c, const unsigned char * a, const unsigned char * b, size_t uBlockLen ) {
  size_t u;
  for ( u = 0; u < uBlockLen; ++u )
    c[u] = a[u] - b[u];
}
#endif


#if defined(SSE2_AVAILABLE)
static inline void decode( __m128i * c, const __m128i * a, size_t uBlockLen ) {
  size_t u;
  for ( u = 0; u < uBlockLen; ++u ) {
    c[u] = _mm_add_epi8( c[u], a[u] );
  }
}
#elif USE_64BIT_ARITH
static inline void decode( uint64_t * c, const uint64_t * a, size_t uBlockLen ) {
  size_t u;
  for ( u = 0; u < uBlockLen; ++u ) {
    /* wp[u] = wp[u] + rp[u]; */
    c[u] = ( ( ( c[u] & UINT64_C(0x00FF00FF00FF00FF) )
              +( a[u] & UINT64_C(0x00FF00FF00FF00FF) )
             )        & UINT64_C(0x00FF00FF00FF00FF)
           )
         | ( ( ( c[u] & UINT64_C(0xFF00FF00FF00FF00) )
              +( a[u] & UINT64_C(0xFF00FF00FF00FF00) )
             )        & UINT64_C(0xFF00FF00FF00FF00)
           );
  }
}
#else
static inline void decode( unsigned char * c, const unsigned char * a, size_t uBlockLen ) {
  size_t u;
  for ( u = 0; u < uBlockLen; ++u )
    c[u] += a[u];
}
#endif

#pragma GCC pop_options



static
void usage() {
  fputs("Usage: srdshashencode [-v][-h][-B <bufferSize>][-c|-d][-l <blockLength>] [-i <input>] [-o <output>]\n", stderr);
  fputs("  sorted raw data set hash coding\n", stderr);
  fputs("  encoding preconditons sorted hash data for better compression\n", stderr);
  fputs("  -v     verbose output\n", stderr);
  fputs("  -h     print usage\n",stderr);
  fputs("  -B <v> bufferSize in kBytes\n", stderr);
  fputs("  -c     encode data (=default)\n",stderr);
  fputs("  -d     decode data\n",stderr);
  fputs("  -l <v> length of each raw data set block in bytes (= cycle length, 20 for SHA-1)\n", stderr);
  fputs("  -i <f> input from file. default is stdin\n", stderr);
  fputs("  -o <f> output to file. default is stdout\n", stderr);
}


int main(int argc, char *argv[]) {
  FILE * inp = stdin;
  FILE * out = stdout;
  const char * outfn = NULL;
  int helpFlag = 0, verboseFlag = 0;
  int encodeFlag = 1;
  int optFlag, k, ret = 0;
  size_t vBufSize = 0;
  void * rdBuffer = NULL;
  void * wrBuffer = NULL;
  void * vbuf[4];
#if defined(SSE2_AVAILABLE)
  __m128i * ubuf[4];
#elif USE_64BIT_ARITH
  uint64_t * ubuf[4];
#else
  unsigned char * ubuf[4];
#endif


  size_t rd, wr;
  extern int optind;

  /* parse command line options */
  while ((optFlag = getopt(argc, argv, "vhB:cdxl:i:o:")) > 0 && optFlag != '?') {
    switch(optFlag) {
    case 'v': ++verboseFlag; break;
    case 'h': ++helpFlag; break;
    case 'B': vBufSize = (size_t)( atol(optarg) * 1024 ); break;
    case 'c': encodeFlag = 1; break;
    case 'd': encodeFlag = 0; break;
    case 'l':
      blockSize = atoi(optarg);
      if ( verboseFlag >= 2 )
        fprintf(stderr, "parsed block length %d\n", blockSize);
      break;
    case 'i':
      inp = fopen(optarg, "rb");
      if (!inp) {
        fprintf(stderr, "error opening input file '%s'\n", optarg);
        return 10;
      }
      break;
    case 'o':
      outfn = optarg;
      break;
    }
  }

  if ( blockSize == -1 ) {
    fprintf(stderr, "error: option '-l' for blockLength is mandatory!\n");
    usage();
    return 10;
  }
  if (optFlag == '?' || helpFlag) {
    usage();
    exit(2);
  }

  if (verboseFlag)
    fprintf(stderr, "using blockSize %d\n", blockSize );

  if ( blockSize <= 0 ) {
    fprintf(stderr, "error: blockSize %d is <= 0 !\n", blockSize);
    return 10;
  }

  if ( outfn )
  {
    out = fopen(outfn, "wb");
    if (!out) {
      fprintf(stderr, "error opening output file '%s'\n", outfn);
      return 10;
    }
  }

  const size_t blkMemSize = (blockSize + 15) & (~15); // round up to 16 byte blocks
#if defined(SSE2_AVAILABLE)
  const size_t uBlockLen = (blockSize + sizeof(__m128i) -1) / sizeof(__m128i);
#elif USE_64BIT_ARITH
  const size_t uBlockLen = (blockSize + sizeof(uint64_t) -1) / sizeof(uint64_t);
#else
  const size_t uBlockLen = blockSize;
#endif

  vbuf[0] = malloc( blkMemSize );
  vbuf[1] = malloc( blkMemSize );
  vbuf[2] = malloc( blkMemSize );
#if defined(SSE2_AVAILABLE)
  ubuf[0] = (__m128i *)vbuf[0];
  ubuf[1] = (__m128i *)vbuf[1];
  ubuf[2] = (__m128i *)vbuf[2];
#elif USE_64BIT_ARITH
  ubuf[0] = (uint64_t *)vbuf[0];
  ubuf[1] = (uint64_t *)vbuf[1];
  ubuf[2] = (uint64_t *)vbuf[2];
#else
  ubuf[0] = (unsigned char *)vbuf[0];
  ubuf[1] = (unsigned char *)vbuf[1];
  ubuf[2] = (unsigned char *)vbuf[2];
#endif

  memset( vbuf[0], 0, blkMemSize );
  k = 0;

#if defined(SSE2_AVAILABLE)
  if (verboseFlag >= 2)
    fprintf(stderr, "using SSE2 intrinsics\n");
#endif

  /* use delta algorithm
   * compression over delta-algorithm produces smaller files
   * compared to easier and symmetric xor !
   */

  /*
   * init:
   * k = 0
   * [0] = 00
   *
   * encode iterations:                     decode iteration:
   * j = k      // old is j
   * k = 1 - k  // new is k
   * [k] = read()                           [1] = read
   * [2] = [k] - [j] --> write()            [0] = [1] + [0] --> write()
   *
   *
   * in iteration 1:
   * j = 0; k = 1
   * [1] = read(), eg. A                    [1] = read == A
   * [2] = [1]-[0]: A-0=A --> write()       [0] = [1]+[0]: A+0 = A ==> OK
   *
   * in iteration 2:
   * j = 1; k = 0
   * [0] = read(), eg. B                    [1] = read() == B-A
   * [2] = [0]-[1]: B-A --> write()         [0] = [1]+[0]: (B-A)+A = B ==> OK
   *
   * in iteration 3:
   * j = 0; k = 1
   * [1] = read(), eg. C                    [1] = read() == C-B
   * [2] = [1]-[0]: C-B --> write()         [0] = [1]+[0]: (C-B)+B = C ==> OK
   *
   * in iteration 4:
   * j = 1; k = 0
   * [0] = read(), eg. D                    [1] = read() == D-C
   * [2] = [0]-[1]: D-C --> write()         [0] = [1]+[0]: (D-C)+C = D ==> OK
   *
   */

  {
    size_t bufferSize = vBufSize ? vBufSize
                                 : (4*blockSize > 65536)
                                   ? (4*blockSize)
                                   : 65536;
    rdBuffer = malloc( bufferSize );
    wrBuffer = malloc( bufferSize );
    if (rdBuffer) setbuffer( inp, rdBuffer, bufferSize );
    if (wrBuffer) setbuffer( out, wrBuffer, bufferSize );
  }

  if (encodeFlag) {

    while ( !feof(inp) ) {
      const int j = k;
      k = 1 - k;

      rd = fread( vbuf[k], blockSize, 1, inp );
      if (!rd) {
        if (!feof(inp)) {
          fprintf(stderr, "error reading from input!\n");
          ret = 9;
        }
        break;
      }

      encode( ubuf[2], ubuf[k], ubuf[j], uBlockLen );

      wr = fwrite( vbuf[2], blockSize, 1, out );
      if (!wr) {
        int ferr = ferror(out);
        if (ferr)
          fprintf(stderr, "error %d writing to output!: %s\n", ferr, strerror(ferr));
        else
          fprintf(stderr, "error %d writing to output!\n", ferr);
        ret = 8;
        break;
      }
    }
  }
  else {
    /* decode */
#if defined(SSE2_AVAILABLE)
  __m128i * rp = ubuf[1];
  __m128i * wp = ubuf[0];
#elif USE_64BIT_ARITH
    uint64_t * rp = ubuf[1];
    uint64_t * wp = ubuf[0];
#else
    unsigned char * rp = ubuf[1];
    unsigned char * wp = ubuf[0];
#endif
    while ( !feof(inp) ) {

      rd = fread( rp, blockSize, 1, inp );
      if (!rd) {
        if (!feof(inp)) {
          fprintf(stderr, "error reading from input!\n");
          ret = 9;
        }
        break;
      }

      decode( wp, rp, uBlockLen );

      wr = fwrite( wp, blockSize, 1, out );
      if (!wr) {
        int ferr = ferror(out);
        if (ferr)
          fprintf(stderr, "error %d writing to output!: %s\n", ferr, strerror(ferr));
        else
          fprintf(stderr, "error %d writing to output!\n", ferr);
        
        ret = 8;
        break;
      }
    }
  }

  if ( out != stdout ) {
    fclose(out);
    free( wrBuffer );
  }
  if ( inp != stdin ) {
    fclose(inp);
    free( rdBuffer );
  }

  return ret;
}
