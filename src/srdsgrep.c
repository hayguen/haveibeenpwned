/*
 * srdsgrep (sorted raw data set grep)
 *
 * derived from sgrep (sorted grep),
 * see http://sgrep.sourceforge.net/
 * see https://sourceforge.net/projects/sgrep/
 * see http://struct.cc/blog/2011/08/10/the-power-of-sorted-grep/
 *
 * Copyright 2017 Hayati Ayguen.  Distributed under the terms
 * of the GNU General Public License (GPL)
 *
 * Copyright 2009 Stephen C. Losen.  Distributed under the terms
 * of the GNU General Public License (GPL)
 *
 * srdsgrep (sorted raw data set grep) is a much faster alternative to traditional Unix
 * grep, but with significant restrictions:
 * 1) All input files must be sorted regular (binary) files.
 * 2) every 'line' is a (binary) block (or line) - all with same fixed length
 * 3) No regular expression support.
 *
 * srdsgrep uses a binary search algorithm, which is very fast, but
 * requires sorted input.  Each iteration of the search eliminates
 * half of the remaining input.  In other words, doubling the size
 * of the file adds just one iteration.
 *
 * srdsgrep seeks to the center of the file,
 *   and places the file pointer at block boundary.
 * srdsgrep compares the search key with the block's key.
 * If the key is greater than the block, then
 *   the process repeats with the second half of the file.
 * If less than, then
 *   the process repeats with the first half of the file.
 * If equal, then
 *   the key matches, but it may not be the earliest match,
 *   so the process repeats with the first half of the file.
 * This is the binary search algorithm,
 *   see https://en.wikipedia.org/wiki/Binary_search_algorithm
 * Eventually all of the input is eliminated and srdsgrep finds
 *   either no matching block  or the first matching block.
 * srdsgrep outputs matching blocks until it encounters a non matching block.
 *
 * Usage: see below at usage()
 *
 * Author:  Hayati Ayguen
 * Author:  Stephen C. Losen   University of Virginia
 */


/* large file support */

#ifdef _AIX
#define _LARGE_FILES
#else
#define _FILE_OFFSET_BITS 64
#endif

#include <sys/stat.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define DBGOUT  0

static int blockSize = -1;
static int keyBeg = 0;
static int keyEnd = -1;
static int keyLen = -1;
static size_t readBlockBuf = 0;
static int verboseFlag = 0;

static unsigned char * keyBuf = NULL;
static unsigned char * blockBuf = NULL;

/* returns length in number of hexadecimal digits - might be odd! */
static
int hashLen( const char * s )
{
  int len = 0;
  int idx = 0;
  while ( s[idx] )
  {
    if ( isxdigit(s[idx]) )
      ++len;
    else if ( isblank(s[idx]) )
      ;  /* ignore space/tab */
    else if ( isspace(s[idx]) )
      break;
    else
    {
      len = -1; /* error */
      break;
    }
    ++idx;
  }
#if 0 && DBGOUT
  if ( len <= 0 || (len & 1) )
    fprintf(stderr, "line '%s' has hashLen %d\n", s, len);
#endif
  return len;
}


/* returns number of converted bytes */
static
int convertHash( const char * s, size_t maxBinLen, unsigned char * bin )
{
  int xlen = 0;
  int blen = 0;  /* 0 length is no error - simply empty line */
  int idx = 0;
  int v = 0;
  while ( s[idx] )
  {
    if ( isxdigit(s[idx]) )
    {
      char c = s[idx];
      v = v << 4;
      if ( c >= '0' && c <= '9' )
        v = v | ( c - '0' );
      else if ( c >= 'A' && c <= 'F' )
        v = v | ( 10 + c - 'A' );
      else if ( c >= 'a' && c <= 'f' )
        v = v | ( 10 + c - 'a' );
      ++xlen;
      if ( !(xlen & 1) )
      {
        if ( blen >= maxBinLen )
        {
          blen = -1; /* error */
          break;
        }
        bin[ blen++ ] = (unsigned char)(v);
        v = 0;
      }
    }
    else if ( isblank(s[idx]) )
      ;  /* ignore space/tab */
    else if ( isspace(s[idx]) )
      break;
    else
    {
      blen = -1; /* error */
      break;
    }
    ++idx;
  }
  if ( xlen & 1 )
  {
#if 0 && DBGOUT
    fprintf(stderr, "line '%s' has %d odd number of hexadecimals!\n", s, xlen);
#endif
    blen = -1; /* error */
  }
#if 0 && DBGOUT
  if ( blen != (int)maxBinLen )
    fprintf(stderr, "line '%s' has binLen %d\n", s, blen);
#endif
  return blen;
}


static inline
int compare(FILE *fp)
{
  readBlockBuf = fread( blockBuf, blockSize, 1, fp );
  if ( readBlockBuf == 1 )
    return memcmp( keyBuf, blockBuf+keyBeg, keyLen * sizeof(unsigned char) );
  else
    return -1;
}


/*
 * Use binary search to find the first matching line and return
 * its byte position.
 */

static off_t
binsrch(FILE *fp, int reverse) {
    off_t low, med, high, prev = -1, ret = -1;
    int cmp;
    struct stat st;

    fstat(fileno(fp), &st);
    high = st.st_size - blockSize;
    low = 0;
    while (low <= high) {
        med = (high + low) / 2;
        /* to start of next line if not at beginning of file */
        med = med / blockSize;
        med = med * blockSize;

        fseeko(fp, med, SEEK_SET);

        /* compare key with current line */
        if (med != prev) {        /* avoid unnecessary compares */
            cmp = compare(fp);
            if (reverse) {
                cmp = -cmp;
            }
            prev = med;
        }

#if DBGOUT
        fprintf(stderr, "binsrch(): lo = %u, mid = %u, hi = %u  ==>  %d\n", (unsigned)low, (unsigned)med, (unsigned)high, cmp);
#endif

        /* eliminate half of input */

        if (cmp < 0) {
            high = med - blockSize;
        }
        else if (cmp > 0) {
            low = med + blockSize;
        }
        else {             /* success, look for earlier match */
            ret = med;
            high = med - blockSize;
        }
    }

#if DBGOUT
    fprintf(stderr, "binsrch(): 1st match at offset %u\n", (unsigned)ret);
#endif
    return ret;
}

/* print all lines that match the key or else just the number of matches */

static void
printmatch(FILE *fp, off_t start,
    const char *fname, int cflag, int maxcount)
{
  int count = 0;

  for ( ; start >= 0 && ( maxcount < 0 || count < maxcount ); ) {
    fseeko(fp, start, SEEK_SET);
    if ( !compare(fp) )
    {
      ++count;
#if 0
      if (!cflag && fname) {
        fputs(fname, stdout);
        fputc(':', stdout);
      }
#endif
      start += blockSize;
      if (readBlockBuf && !cflag)
      {
        size_t w = fwrite( blockBuf, blockSize, 1, stdout );
        if ( w != 1 ) {
          fprintf(stderr, "Error writing all matches to output!\n");
          break;
        }
#if DBGOUT
fprintf(stdout, "\n");
#endif
      }
      if (feof(fp)) {
        break;
      }
    }
    else
      break;
  }
  if (cflag) {
    if (fname) {
      fputs(fname, stdout);
      fputc(':', stdout);
    }
    printf("%d\n", count);
  }
}

static
void usage() {
  fputs("Usage: srdsgrep [-v][-h][-c][-m <max>][-r][-l <blockLength>][-b <keyBegin>][-e <keyEnd>] [-x] key [ sorted_file ... ]\n", stderr);
  fputs("  sorted raw data set grep\n", stderr);
  fputs("  -v     verbose output\n", stderr);
  fputs("  -h     print usage\n",stderr);
  fputs("  -c     print count matches - not matching contents\n", stderr);
  fputs("  -m <v> stop reading file after N matches. default is no stop.\n", stderr);
  fputs("  -B <v> bufferSize in kBytes\n", stderr);
  fputs("  -r     sorted file is reversed (descending) order\n", stderr);
  fputs("  -l <v> length of each binary block in bytes\n", stderr);
  fputs("  -b <v> key's begin offset inside block\n", stderr);
  fputs("  -e <v> key's end offset inside block\n", stderr);
  fputs("  -x     key (or key file content) is hexadecimal, e.g. FFAA01\n", stderr);
  /* fputs("  -f     key is in file. key parameter is filename\n", stderr); */
}


int main(int argc, char **argv)
{
  const char *keyarg = 0;
  int i, numfile, status;
  int helpFlag = 0;
  int countFlag = 0, revFlag = 0, hexFlag = 0, fileFlag = 0, maxcount = -1;
  int changedKeyOrBlock = 0;
  off_t where;
  size_t vBufSize = 0;
  void * rdBuffer = NULL;
  struct stat st;
  extern int optind;

  /* parse command line options */
  while ((i = getopt(argc, argv, "vhB:crxm:l:b:e:")) > 0 && i != '?') {
    switch(i) {
    case 'v': ++verboseFlag; break;
    case 'h': ++helpFlag; break;
    case 'B': vBufSize = (size_t)( atol(optarg) * 1024 ); break;
    case 'c': ++countFlag; break;
    case 'r': ++revFlag; break;
    case 'x': ++hexFlag; break;
    /* case 'f': ++fileFlag; break; */
    case 'm': maxcount = atoi(optarg);  break;
    case 'l':
      blockSize = atoi(optarg);
      if ( verboseFlag >= 2 )
        fprintf(stderr, "parsed block length %d\n", blockSize);
      break;
    case 'b':
      keyBeg = atoi(optarg);
      if ( verboseFlag >= 2 )
        fprintf(stderr, "parsed key Begin %d\n", keyBeg);
      break;
    case 'e':
      keyEnd = atoi(optarg);
      if ( verboseFlag >= 2 )
        fprintf(stderr, "parsed key End %d\n", keyEnd);
      break;
    }
  }
  if (i == '?' || helpFlag || optind >= argc) {
    usage();
    exit(2);
  }
  i = optind;
  keyarg = argv[i++];

  if (!fileFlag) {
    if (!hexFlag) {
      int len = strlen(keyarg);
      if ( keyEnd <= 0 )
      {
        keyEnd = keyBeg + len -1;
        if ( verboseFlag >= 2 )
          fprintf(stderr, "strlen(keyarg) = %d == keyLen => keyEnd = %d\n", len, keyEnd );
        keyBuf = (unsigned char *)malloc( len * sizeof(unsigned char) );
        memcpy( keyBuf, keyarg, len * sizeof(unsigned char) );
      }
      else {
        int alen = keyEnd - keyBeg + 1;
        keyBuf = (unsigned char *)malloc( alen * sizeof(unsigned char) );
        memset( keyBuf, 0, alen * sizeof(unsigned char) );
        int clen = ( alen < len ) ? alen : len;
        if ( verboseFlag >= 2 )
          fprintf(stderr, "alen = %d, clen = %d\n", alen, clen);
        memcpy( keyBuf, keyarg, clen * sizeof(unsigned char) );
      }
    }
    else {
      int hlen = hashLen(keyarg);
      int len = hlen / 2;
      if ( keyEnd <= 0 )
      {
        keyEnd = keyBeg + len -1;
        keyBuf = (unsigned char *)malloc( len * sizeof(unsigned char) );
        convertHash( keyarg, len, keyBuf );
      }
      else {
        int alen = keyEnd - keyBeg + 1;
        keyBuf = (unsigned char *)malloc( alen * sizeof(unsigned char) );
        memset( keyBuf, 0, alen * sizeof(unsigned char) );
        int clen = ( alen < len ) ? alen : len;
        convertHash( keyarg, clen, keyBuf );
      }
    }
  }

  keyLen = keyEnd - keyBeg + 1;

  if (verboseFlag)
    fprintf(stderr, "using key at offset %d with length %d at blockSize %d\n", keyBeg, keyLen, blockSize );

  if ( keyEnd < 0 && blockSize > 0 )
  {
    keyEnd = blockSize -1;
    keyLen = keyEnd - keyBeg + 1;
    changedKeyOrBlock = 1;
  }

  if ( blockSize <= 0 ) {
    blockSize = keyBeg + keyLen;
    changedKeyOrBlock = 1;
    if (verboseFlag)
      fprintf(stderr, "info: using block size %d\n", blockSize);
  }
  else if ( blockSize < keyLen ) {
    fprintf(stderr, "error: blockSize %d is smaller than key length %d !\n", blockSize, keyLen);
    return 10;
  }

  if (changedKeyOrBlock && verboseFlag)
    fprintf(stderr, "using key at offset %d with length %d at blockSize %d\n", keyBeg, keyLen, blockSize );

  if ( blockSize <= 0 ) {
    fprintf(stderr, "error: blockSize %d is <= 0 !\n", blockSize);
    return 10;
  }
  if ( keyLen <= 0 ) {
    fprintf(stderr, "error: keyLen %d is <= 0 !\n", keyLen);
    return 10;
  }
  if ( keyEnd <= 0 ) {
    fprintf(stderr, "error: keyEnd %d is <= 0 !\n", keyEnd);
    return 10;
  }


  blockBuf = (unsigned char *)malloc( blockSize * sizeof(unsigned char) );

  /* if no input files, then search stdin */

  if ((numfile = argc - i) == 0) {
    size_t bufferSize = vBufSize ? vBufSize : 65536;
    fstat(fileno(stdin), &st);
    if ((st.st_mode & S_IFREG) == 0) {
      fputs("srdsgrep: STDIN is not a regular file\n", stderr);
      exit(2);
    }
    rdBuffer = malloc( bufferSize );
    if (rdBuffer) setbuffer( stdin, rdBuffer, bufferSize );

    where = binsrch(stdin, revFlag);
    printmatch(stdin, where, 0, countFlag, maxcount);

    exit(where < 0);
  }

  /* search each input file */
  for (status = 1; i < argc; i++) {
    size_t bufferSize = vBufSize ? vBufSize : 65536;
    FILE *fp = fopen(argv[i], "rb");
    if ( !fp ) {
      fprintf(stderr, "srdsgrep: could not open %s\n", argv[i]);
      status = 2;
      continue;
    }
    fstat(fileno(fp), &st);
    if ((st.st_mode & S_IFREG) == 0) {
      fprintf(stderr, "srdsgrep: %s is not a regular file\n", argv[i]);
      status = 2;
      fclose(fp);
      continue;
    }

    rdBuffer = malloc( bufferSize );
    if (rdBuffer) setbuffer( fp, rdBuffer, bufferSize );

    where = binsrch(fp, revFlag);
    printmatch(fp, where, numfile == 1 ? 0 : argv[i], countFlag, maxcount);
    if (status == 1 && where >= 0) {
      status = 0;
    }
    fclose(fp);
    free( rdBuffer );
  }
  exit(status);
}
