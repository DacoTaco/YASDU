# R.N.D
R.N.D ( **R**andom **N**and **D**ecryptor ) is a tool i made to decrypt raw nand dumps of the encrypted partitions ( PRODINFO , PRODINFOF,SAFE,SYS & USER)
using kokke's AES code & luigoalma's code which is based on plutoo's code.

you need the biskeydump.txt here as well.

this is confirmed working on Windows & Linux.

in linux the dumps can be mounted and checked
in windows it would first need to be decently(including added GPT or MBR) converted to vhd so you can mount it in windows.
this is something i want to expand on in the future but lack time for right now


no special tools needed for this besides make and gcc :)
