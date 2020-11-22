#     D.a.n

## AAAHHH WAS IST DAS
**D**ecrypted **A**ccess of **N**and is a tool used to allow access to the nand, decrypted
D.a.n sets up a FUSE file system that contains a few files, which are the NAND partitions decrypted. 
currently only uses bin dumps in the same directory as D.a.n but is planned to be used on a life switch

## Can i use this?

YES! Yes you can. for it to work you will need Fuse & libfuse-dev

check if you have these by running the following command
```
fusermount --v
```

if a command unknown error pops up, install it using your package manager
```
sudo pacman -Syu fuse libfuse-dev
```
or for debian : 
```
sudo apt-get install fuse libfuse-dev
```

once installed, clone YASDU like always

```
git clone https://github.com/DacoTaco/YASDU.git
```

compile the code by running make and start D.a.n by running the following command : 

```
sudo ./Dan 
```

normally, you should see the keys being detected, the partitions it found and where it added them ( this should be /mnt/nand/ )
if you want to use it with local dumps (NAND, PRODINFO, USER, ...), add a -l at the end of the command.
if you want the partitions to be mounted in read-write mode, add -rw to the command.

enjoy!

## i'm getting a BIS key error!
ow ye, i forgot didn't i? you need your bis keys dumped in a format used by [biskeydump](https://github.com/rajkosto/biskeydump) in a txt called biskeydump.txt.

format : 
```
BIS KEY 0 (crypt): [crypt key 0]
BIS KEY 0 (tweak): [tweak key 0]
BIS KEY 1 (crypt): [crypt key 1]
BIS KEY 1 (tweak): [tweak key 1]
...
etc etc
```
