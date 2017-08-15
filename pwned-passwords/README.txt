
"Pwned Passwords" is a website and online service from Troy Hunt,
allowing to check passwords, if they were already pawned.
See following links:

https://haveibeenpwned.com/Passwords
https://www.troyhunt.com/introducing-306-million-freely-downloadable-pwned-passwords/

The bash script "is-pwned-password.sh", provided here,
is to allow checking passwords locally (offline) without internet,
utilizing the password databases also provided from Troy Hunt.


Following instructions guide through download of the database and setup of required tools.

* this README is part of srds-tools, which should already been installed
see https://github.com/hayguen/srds-tools


* change directory to srds-tools/pwned-passwords

cd srds-tools/pwned-passwords


* other tools, like sort, sha1sum, od, 7z and wget, are required. install them with:

sudo apt-get install coreutils wget p7zip-full


* download and prepare the password database with

./get-pwned-passwords.sh


* now you can use is-pwned-password.sh:

./is-pwned-password.sh password

in case password contains spaces or even '$' characters use

./is-pwned-password.sh 'password'

call without parameters


* you might remove temporary files with

./rm-temporary-files.sh

if you need that space


