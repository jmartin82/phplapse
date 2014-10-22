# PHPLAPSE #

PHPlapse is a zend extension that allow analize part of your script execution flow. You can use it for understand how your script works or find a little mistakes in your code.

Currently the extension is in very alpha state, not use in production enviroment.

## Install ##

Depedencies (Debian flavour):

apt-get install gcc php php5-dev

__Build__

$ phpize
$ ./configure
$ make

__Install__

make install
Add extension in php.ini
zend_extension=phplapse.so

Check installation:
php-config|grep extension-dir

## Functions ##

boolean phplapse_start();

__Description__

Start the lapse log.

***

string phplapse_stop();

__Description__

Stop the lapse log and return the lapse index file.

Return Values

Lapse index file path.


## Output ##

The phplapse extension generates two files with useful execution information for understand the script flow or for debugging purpose.

The first file with extension .idx contains all lapse steps information and contains references to .dat file.

The .dat file contains context of the every steps.

__.idx file format__

HEADER
```
╔═════════╦═══════════════════════╦═════════════════╦═════════╗
║ Version ║        dFile          ║      cDate      ║  steps  ║
╟─────────╫───────────────────────╫─────────────────╫─────────╢
║ 2 bytes ║       70 bytes        ║    20 bytes     ║ 4 bytes ║
╚═════════╩═══════════════════════╩═════════════════╩═════════╝
```

Version: File format version
dFile: File name of dat file. (The same of the idx file with differnt extension)
cDate: Creation date
steps: Analyzed steps

STEP(n)
```
╔═════════╦═════════╦═════════════╦═════════╦═════════╦════════════════╦════════════════╦═════════════╗
║  Step   ║   Time  ║    tTime    ║   Mem   ║ memPeak ║  Function      ║ Class          ║ cReference  ║
╟─────────╫─────────╫─────────────╫─────────╫─────────╫────────────────╫────────────────╫─────────────╢
║ 4 bytes ║ 4 bytes ║   4 bytes   ║ 4 bytes ║ 4 bytes ║  20 bytes      ║  20 bytes      ║ 4 bytes     ║
╚═════════╩═══════════════════════╩═════════╩═════════╩════════════════╩════════════════╩═════════════╝
```

Step: Step number
Time: Time used in step (micro seconds)
tTime: Total lapse time (milli seconds)
Mem: Memory used in step (Kb)
memPeak: Memory peak (kb)
Function: Function executed in step
Class: Class executed in step
cReference: Context byte reference in .dat file.

Note: All numbers are in "Little Endian".

__.dat file Format__

```
╔═════════════╦══════════════════════════════════════════════════════╗
║ LN1         ║  Full path of file                                   ║
╟─────────────╫──────────────────────────────────────────────────────╢
║ LN2         ║  Execueted line in file                              ║
╟─────────────╫──────────────────────────────────────────────────────╢
║ LN3 - LN 15 ║  Context of line (5 lines before and 5 line after)   ║
╚═════════════╩══════════════════════════════════════════════════════╝
```

Note: All lines finish with "\n" scape character.

## How it works ##

Between phplapse_start and phplapse_stop functions, the extension record every line executed by php engine. In phplapse domain specific language every line will be enclosed in steps and every step contain some important information about execution context for example memory and time consumed, etc...

## Example ##

```php
<?php
phplapse_start();
class Dog {
        var $name;
        var $age;
        var $owner;

        function __construct($name,$age,$owner) {
                $this->name = $name;
                $this->age = $age;
                $this->owner = $owner;
                }

        function getAge()
                {
                return $this->age;
                }
        function getOwner()
                {
                return ucwords($this->owner);
                }
        function getName()
                {
                return ucwords($this->name);
                }
}
$myDog = new Dog("Nik",2,"Jordi");
echo $myDog->getName();
echo "\n";
echo $myDog->getAge();
echo "\n";
echo $myDog->getName();
echo "\n";
$test_file = phplapse_stop();
echo "IDX file: $test_file\n";
?>
```

##Reader application example##

This repository includes too a sample application to read index files. This aplication was made in python an needs urwid libs.


__Run__

sudo apt-get install python-pip
pip install urwid
LapseReader -i index_file.idx [-p path_of_data_file]

![LapseReader in action](https://raw.githubusercontent.com/jmartin82/phplapse/master/lapse_reader.png "LapseReader")
