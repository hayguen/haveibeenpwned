/*
 * srdsmerge (sorted raw data set merge)
 *
 * Copyright 2017 Hayati Ayguen.  Distributed under the terms
 * of the GNU General Public License (GPL)
 *
 * srdsmerge (sorted raw data set merge) is an alternative to traditional Unix
 * sort with option "-m", for sorted raw data set files.
 * Limitations / requirements:
 * 1) All input files must be sorted regular files.
 * 2) every 'line' is a raw data set (block) - all with same fixed length
 *
 * Usage: see below at usage()
 *
 * Author:  Hayati Ayguen
 */


#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define MAXINFILES  16

static int blockSize = -1;
static int keyBeg = 0;
static int keyEnd = -1;
static int keyLen = -1;
static int verboseFlag = 0;

static FILE * input[MAXINFILES];
static void * rdBuffers[MAXINFILES];
static size_t readBlockBuf[MAXINFILES];
static unsigned char * blockBuf[MAXINFILES];

static
void usage() {
  fputs("Usage: srdsmerge [-v][-h][-r][-l <blockLength>][-b <keyBegin>][-e <keyEnd>][-o <output>] (<sorted_file>)+\n", stderr);
  fputs("  sorted raw data set merge\n", stderr);
  fputs("  -v     verbose output\n", stderr);
  fputs("  -h     print usage\n",stderr);
  fputs("  -B <v> bufferSize in kBytes\n", stderr);
  fputs("  -r     sorted files are reversed (descending) order\n", stderr);
  fputs("  -l <v> length of each raw data set block in bytes\n", stderr);
  fputs("  -b <v> key's begin offset inside block\n", stderr);
  fputs("  -e <v> key's end offset inside block\n", stderr);
  fputs("  -o <f> output to file. default is stdout\n", stderr);
  fputs("  sorted_file  minimum 2 filenamess required\n", stderr);
}


int main(int argc, char *argv[]) {
  FILE * out = stdout;
  const char * outfn = NULL;
  int optFlag, numInputs = 0;
  int numAvailable = 0;
  int firstAvailable = -1;
  int lastAvailable = -1;
  int isAvailable = 0;
  int helpFlag = 0;
  int revFlag = 0;
  int changedKeyOrBlock = 0;
  size_t vBufSize = 0;
  size_t bufferSize = 0;
  void * wrBuffer = NULL;
  extern int optind;

  /* parse command line options */
  while ((optFlag = getopt(argc, argv, "vhB:rl:b:e:o:")) > 0 && optFlag != '?') {
    switch(optFlag) {
    case 'v': ++verboseFlag; break;
    case 'h': ++helpFlag; break;
    case 'B': vBufSize = (size_t)( atol(optarg) * 1024 ); break;
    case 'r': ++revFlag; break;
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
    case 'o':
      outfn = optarg;
      break;
    }
  }
  if (optFlag == '?' || helpFlag) {
    usage();
    exit(2);
  }
  optFlag = optind;

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

  bufferSize = vBufSize ? vBufSize : 65536;

  /* search each input file */
  for ( ; optFlag < argc; optFlag++) {
    FILE * fp = fopen(argv[optFlag], "rb");
    if (!fp) {
        fprintf(stderr, "srdsmerge:  could not open %s\n", argv[optFlag]);
        exit(2);
    }

    input[numInputs] = fp;
    rdBuffers[numInputs] = malloc( bufferSize );
    if (rdBuffers[numInputs]) setbuffer( fp, rdBuffers[numInputs], bufferSize );
    blockBuf[numInputs] = (unsigned char *)malloc( blockSize * sizeof(unsigned char) );
    readBlockBuf[numInputs] = fread( blockBuf[numInputs], blockSize, 1, input[numInputs] );
    isAvailable = (int)( readBlockBuf[numInputs] );
    numAvailable += isAvailable;
    if ( isAvailable ) {
      if ( firstAvailable < 0 )
        firstAvailable = numInputs;
      lastAvailable = numInputs;
    }
    ++numInputs;
  }

  /* if no input files? */
  if (numInputs == 0) {
    fputs("srdsmerge: STDIN is not accepted. input filenames required at command line!\n", stderr);
    exit(10);
  }
  else if ( numInputs < 2 )
  {
    fputs("srdsmerge: minimum 2 input filenames required!\n", stderr);
    exit(9);
  }

  if ( numInputs && numAvailable && outfn )
  {
    out = fopen(outfn, "wb");
    if (!out) {
      fputs("error opening output file!\n", stderr);
      exit(8);
    }
  }

  wrBuffer = malloc( bufferSize );
  if (wrBuffer) setbuffer( out, wrBuffer, bufferSize );

  while ( numAvailable )
  {
    size_t w;
    int fno, cmp;

    /* search for fileNumber fno with best block key */
    int fbest = firstAvailable;
    if ( !revFlag ) {
      for ( fno = fbest+1; fno <= lastAvailable; ++fno ) {
        if ( !readBlockBuf[fno] )
          continue;
        cmp = memcmp( blockBuf[fno] +keyBeg, blockBuf[fbest] +keyBeg, keyLen );
        if ( cmp < 0 )
          fbest = fno;
      }
    }
    else {
      for ( fno = fbest+1; fno <= lastAvailable; ++fno ) {
        if ( !readBlockBuf[fno] )
          continue;
        cmp = memcmp( blockBuf[fno] +keyBeg, blockBuf[fbest] +keyBeg, keyLen );
        if ( cmp > 0 )
          fbest = fno;
      }
    }

    // output best block
    w = fwrite( blockBuf[fbest], blockSize, 1, out );
    if (!w) {
      fputs("error writing to output file!\n", stderr);
      exit(7);
    }

    // load next block of best file
    readBlockBuf[fbest] = fread( blockBuf[fbest], blockSize, 1, input[fbest] );
    if ( !readBlockBuf[fbest] )
    {
      // update number of available blocks over all files
      int prevLastAvailable = lastAvailable;
      fno = firstAvailable;
      firstAvailable = lastAvailable = -1;
      for ( numAvailable = 0; fno <= prevLastAvailable; ++fno ) {
        isAvailable = (int)( readBlockBuf[fno] );
        numAvailable += isAvailable;
        if ( isAvailable ) {
          if ( firstAvailable < 0 )
            firstAvailable = fno;
          lastAvailable = fno;
        }
      }
    }

  }

  if ( out != stdout ) {
    fclose(out);
    free( wrBuffer );
  }

  return 0;
}
