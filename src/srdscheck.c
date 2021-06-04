/*
 * srdscheck (check if raw data is sorted)
 *
 * Copyright 2021 Hayati Ayguen.  Distributed under the terms
 * of the GNU General Public License (GPL)
 *
 * srdscheck (check if raw data is sorted) is an alternative to traditional Unix
 * sort with option "-c", for sorted raw data set files.
 * Limitations / requirements:
 * 1) input fils must be sorted regular file
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

static int blockSize = -1;
static int keyBeg = 0;
static int keyEnd = -1;
static int keyLen = -1;
static int verboseFlag = 0;

static FILE * input = NULL;
static void * rdBuffer = NULL;
static size_t readBlockBuf = 0;
static unsigned char * blockBuf[2];

static
void usage() {
  fputs("Usage: srdscheck [-v][-h][-r][-l <blockLength>][-b <keyBegin>][-e <keyEnd>] <file>\n", stderr);
  fputs("  check if raw data set is sorted\n", stderr);
  fputs("  -v     verbose output\n", stderr);
  fputs("  -h     print usage\n",stderr);
  fputs("  -B <v> bufferSize in kBytes\n", stderr);
  fputs("  -r     sorted files are reversed (descending) order\n", stderr);
  fputs("  -l <v> length of each raw data set block in bytes\n", stderr);
  fputs("  -b <v> key's begin offset inside block\n", stderr);
  fputs("  -e <v> key's end offset inside block\n", stderr);
  fputs("  file  filename required\n", stderr);
}


int main(int argc, char *argv[]) {
  FILE * out = stdout;
  unsigned long long dataSetNo = 0;
  int optFlag;
  int numAvailable = 0;
  int readIdx = 0;
  int helpFlag = 0;
  int revFlag = 0;
  int cmp;
  size_t vBufSize = 0;
  size_t bufferSize = 0;
  extern int optind;

  /* parse command line options */
  while ((optFlag = getopt(argc, argv, "vhB:rl:b:e:")) > 0 && optFlag != '?') {
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
    }
  }
  if (optFlag == '?' || helpFlag) {
    usage();
    exit(2);
  }
  optFlag = optind;

  keyLen = keyEnd - keyBeg + 1;

  if ( keyEnd < 0 && blockSize > 0 )
  {
    keyEnd = blockSize -1;
    keyLen = keyEnd - keyBeg + 1;
  }

  if ( blockSize <= 0 ) {
    blockSize = keyBeg + keyLen;
    if (verboseFlag)
      fprintf(stderr, "info: using block size %d\n", blockSize);
  }
  else if ( blockSize < keyLen ) {
    fprintf(stderr, "error: blockSize %d is smaller than key length %d !\n", blockSize, keyLen);
    return 10;
  }

  if (verboseFlag)
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
  if (optFlag < argc) {
    input = fopen(argv[optFlag], "rb");
    if (!input) {
        fprintf(stderr, "srdscheck:  could not open %s\n", argv[optFlag]);
        exit(2);
    }

    rdBuffer = malloc( bufferSize );
    if (rdBuffer) setbuffer(input, rdBuffer, bufferSize);
    blockBuf[0] = (unsigned char *)malloc( blockSize * sizeof(unsigned char) );
    blockBuf[1] = (unsigned char *)malloc( blockSize * sizeof(unsigned char) );
    readBlockBuf = fread(blockBuf[readIdx], blockSize, 1, input);
    if (readBlockBuf)
      ++dataSetNo;
    readIdx = 1 - readIdx;
  }

  /* if no input files? */
  if (!input) {
    fputs("srdscheck: STDIN is not accepted. input filenames required at command line!\n", stderr);
    exit(10);
  }

  while ( readBlockBuf )
  {
    // load next block
    readBlockBuf = fread( blockBuf[readIdx], blockSize, 1, input );
    if (!readBlockBuf)
      break;

    cmp = memcmp( blockBuf[1 - readIdx] +keyBeg, blockBuf[readIdx] +keyBeg, keyLen );
    if (!revFlag)
    {
      if (cmp > 0) {
        if (verboseFlag)
          fprintf(stderr, "raw data set %lu (from 0) is not in ascending order!\n", (unsigned long)dataSetNo);
        return 1;
      }
    }
    else
    {
      if (cmp < 0) {
        if (verboseFlag)
          fprintf(stderr, "raw data set %lu (from 0) is not in descending order!\n", (unsigned long)dataSetNo);
        return 1;
      }
    }

    ++dataSetNo;
  }

  if (verboseFlag)
    fprintf(stderr, "%lu raw data sets are order.\n", (unsigned long)dataSetNo);

  return 0;
}
