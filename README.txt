
* hex2rds   converts text files with hexadecimal codes to raw data set files
* srdsgrep  sorted raw data set grep
* srdsmerge sorted raw data set merge

* convert text/csv files to rds:
  cat input.txt | cut -b 1-20 | od -A n -t x1 -w20 -v | sed 's/ //g'
sed is not necessary, but reduces the output file size

* application / purpose:
pwned-passwords has a sha1-database with 40 characters each line for the 20-byte SHA1 sum.
This text password database requires ~ 12.5 GB.
Stored as raw data, just 20 bytes per data set need to be stored - even without carriage return/newline.
This rds storage compresses the database to half size: ~ 6.2 GB.

Conversion of sorted text database can be done with hex2rds.
Fast binary search over sorted rds is possible with srdsgrep.
Sorted database updates can be achieved with srdsmerge - after converting update with hex2rds.


Usage: hex2rds [-h | --help] [-n <rawSize>] [-i <input>] [-o <output>]
  hex2rds converts text files with hexadecimal (hash) codes to raw data set (rds) files
  every input line must have same even length! spaces are ignored.
  -h | --help   print usage
  -n <rawSize>  tell expected raw length in bytes, e.g. 20 for SHA1
                by default, the 1st line's length will be used
  -i <input>    use input from file. default: stdin
  -o <output>   output to file. default: stdout

Usage: srdsgrep [-v][-h][-c][-m <max>][-r][-l <blockLength>][-b <keyBegin>][-e <keyEnd>] [-x] key [ sorted_file ... ]
  sorted raw data set grep
  -v     verbose output
  -h     print usage
  -c     print count matches - not matching contents
  -m <v> stop reading file after N matches. default is no stop.
  -r     sorted file is reversed (descending) order
  -l <v> length of each binary block in bytes
  -b <v> key's begin offset inside block
  -e <v> key's end offset inside block
  -x     key (or key file content) is hexadecimal, e.g. FFAA01

Usage: srdsmerge [-v][-h][-r][-l <blockLength>][-b <keyBegin>][-e <keyEnd>][-o <output>] (<sorted_file>)+
  sorted raw data set merge
  -v     verbose output
  -h     print usage
  -r     sorted files are reversed (descending) order
  -l <v> length of each raw data set block in bytes
  -b <v> key's begin offset inside block
  -e <v> key's end offset inside block
  -o <f> output to file. default is stdout
  sorted_file  minimum 2 filenamess required
