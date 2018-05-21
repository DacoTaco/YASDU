# YASDU
Yasdu ( **Y**et **A**nother **S**witch **D**umping **U**tility ) is a set of switch linux tool used to dump & access switch data.
i developed this after wanting to dump my NAND, TSEC fw and keys but having only a small SD around. i had tried other tools around but was not pleased as the data was wrong , to big and didn't provide hashes to verify the dump ( \*cough\* https://github.com/daanhenke/httpnand/issues \*cough\* )

there for , i made a python script to do it myself

...and now im just adding tools while im playing with nand :')

#     D.a.n

### AAAHHH WAS IST DAS
**D**ecrypted **A**ccess of **N**and is a tool used to allow access to the nand, decrypted
D.a.n sets up a FUSE file system that contains a few files, which are the NAND partitions decrypted. 
currently only uses bin dumps in the same directory as D.a.n but is planned to be used on a life switch

### Can i use this?

YES! Yes you can. for it to work you will need Fuse & libfuse-dev

check if you have these by running the following command
```
fusermount --v
```

if a command unknown error pops up, install it using your package manager
```
sudo pacman -Syu fuse libfuse-dev
```
or
```
sudo apt-get install fuse libfuse-dev
```

once installed, close YASDU like always

```
git clone https://github.com/DacoTaco/YASDU.git
```

compile the code and run it like this


```
./Dan -f /mnt/mount
```

normally, you should see the keys being detected, the partitions it found and where it added them on the mount

enjoy!


#     NandFileServer

### WHAT IS THIS

the NandFileServer is a python script that sets up a web server that allows the downloading of nand & switch data

### HOW DO I RUN THIS D:

First check if you have python installed on the switch. this can be done using 
```
python --version
```

if you see python and the version running, you have python
if you see something like 'python : command not found' run the following 
```
sudo pacman -Syu python
```
this should install python.

to run Yasdu you first need to get the code from this github. this can be done using
```
git clone https://github.com/DacoTaco/YASDU.git
```
or by downloading the code manually

once downloaded go into the directory

```
cd YASDU
```
and run 

```
sudo python fileServer.py
```
it could take a while because it generates hashes first. 
if you dont want to wait add -s to the python script command (after fileServer.py) so it skips the hashing of the main NAND

once running go to the ip given by the script in a browser and start downloading! 
if you want to verify the dumps afterwards you can use md5sum in linux or fsum in windows

### WHY ARENT THE KEYS ON THE PAGE
i was planning to add the downloading of the system's BIS keys and all other keys but i wasn't able to enable/access the TSEC/Falcon processor from python/linux. i tried poking the registers but it freezes the switch.

i have tried starting from scratch and trying to enable everything needed trough the clock registers but most of them are access denied when accessed through /dev/mem or devmem2 

this is as far as i got and linux is not an OS/system/kernel i know well sadly.

linux knows tsec is there but accessing is not as direct as i had hoped. if anyone can help me out with that, i would love to add that functionality!
