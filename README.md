
# have i been pwned?

## brief description:

this repository [https://github.com/hayguen/haveibeenpwned](https://github.com/hayguen/haveibeenpwned)
contains a bunch of programs and scripts to allow local testing against the pwned passwords
database [https://haveibeenpwned.com/Passwords](https://haveibeenpwned.com/Passwords) - after
having downloaded the SHA-1 database - ordered by hash.

"Pwned Passwords" is an independent website and online service from Troy Hunt,
allowing to check passwords, if they were already pawned.
it inspired to develop the programs and scripts in this repository.
see following links:

* [https://haveibeenpwned.com/Passwords](https://haveibeenpwned.com/Passwords)
* [https://www.troyhunt.com/introducing-306-million-freely-downloadable-pwned-passwords/](https://www.troyhunt.com/introducing-306-million-freely-downloadable-pwned-passwords/])


## setup (for debian/ubuntu) linux:

use following commands in a linux terminal with bash:

```
sudo apt-get install coreutils wget gawk p7zip-full
git clone https://github.com/hayguen/haveibeenpwned.git
cd haveibeenpwned
cmake -B build --config Release -DCMAKE_INSTALL_PREFIX=$HOME/.local src
cmake --build build --target install
```

you might omit `-DCMAKE_INSTALL_PREFIX=$HOME/.local` to install the tools system-wide.
in this case, you need to prepend `sudo ` to the following install command.

in case `$HOME/.local/bin` didn't exist before and wasn't in your PATH,
you might want to add it explicitly into your PATH:
```
export PATH=$PATH:$HOME/.local/bin
```

or simply create a new terminal and `cd` into the cloned git repository `haveibeenpwned`.
usually, the `$HOME/.local/bin` is added automatically.

then continue with the download, which takes some time:
```
cd pwned-passwords
./get-pwned-passwords.sh
./rm-temporary-files.sh
```

read and follow the printed instructions


## checking passwords

open a terminal and `cd` into subdirectory the `pwned-passwords` of the cloned  git repository `haveibeenpwned`.
now you can check with
```
./is-pwned-password.sh 'a-password'
```

e.g.
```
./is-pwned-password.sh 'this$is/my ultra!secret_pwd'
```

interestingly, this is reported to be safe with database V8:
```
password not in database: password is OK
```

negative examples are
```
./is-pwned-password.sh '123'
./is-pwned-password.sh 'secret'
./is-pwned-password.sh 'password'
```


## srds - sorted raw data set

the following isn't necessary, in case you just want to check your passwords.

some small programs - for quick local testing against sorted raw (binary) data sets,
as Troy Hunt's database is. but these programs might be useful for other applications either ..

`srds` is the abbreviation for sorted raw (binary) data set`

`rds` is the abbreviation for (unsorted) raw data set

* `hex2rds`: converts text input (file) with hexadecimal codes to a raw data set file
* `srdsgrep`: sorted raw data set grep
* `srdsmerge`: sorted raw data set merge
* `srdshashencode`: sorted raw data set hash encoding

* convert text/csv files to rds:
```
cat input.txt | cut -b 1-20 | od -A n -t x1 -w20 -v | sed 's/ //g' | hex2rds -o output.rds
```

sed is not necessary, but reduces the output file size, when output shall be stored.


### application / purpose:

pwned-passwords has a sha1-database with 40 characters each line for the 20-byte SHA1 sum.
this uncompressed text password database requires ~ 35 GB with version 8. the compressed 7z occupies ~ 16 GB.
stored as raw data, just 20 bytes per data set needs to be stored - even without carriage return/newline.
this rds storage compresses the database to half of the uncompressed text size: ~ 16 GB,
but gaining seekable direct access - enabling binary search algorithm.

conversion of the sorted text database can be done with hex2rds.
fast binary search over sorted rds is possible with srdsgrep.
sorted database updates can be achieved with srdsmerge - after converting the update with hex2rds.

srdshashencode does 'precondition' (when encoding) a sorted rds file to achieve a better compression ratio:
adjacent datasets are simply differentially encoded.
with version 8 of the password database, the compressed result size is ~ 14 GB: 10% smaller than the original 7z.


```
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

Usage: srdshashencode [-v][-h][-B <bufferSize>][-c|-d][-l <blockLength>] [-i <input>] [-o <output>]
  sorted raw data set hash coding
  encoding preconditons sorted hash data for better compression
  -v     verbose output
  -h     print usage
  -B <v> bufferSize in Bytes
  -c     encode data (=default)
  -d     decode data
  -l <v> length of each raw data set block in bytes (= cycle length, 20 for SHA-1)
  -i <f> input from file. default is stdin
  -o <f> output to file. default is stdout
```
