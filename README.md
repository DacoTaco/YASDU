# YASDU
Yasdu ( **Y**et **A**nother **S**witch **D**umping **U**tility ) is a switch linux tool used to dump switch data over the network.
i developed this after wanting to dump my NAND, TSEC fw and keys but having only a small SD around. i had tried other tools around but was not pleased as the data was wrong , to big and didn't provide hashes to verify the dump ( \*cough\* https://github.com/daanhenke/httpnand/issues \*cough\* )

there for , i made a python script to do it myself

# HOW DO I RUN THIS D:

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
if you dont want to wait add -s to the python command so it skips the hashing of the main NAND

once running go to the ip given by the script in a browser and start downloading! 
if you want to verify the dumps afterwards you can use md5sum in linux or fsum in windows

# WHY ARENT THE KEYS ON THE PAGE
i was planning to add the downloading of the system's BIS keys and all other keys but i wasn't able to enable/access the TSEC/Falcon processor from python/linux. i tried poking the registers but it freezes the switch.
i have tried starting from scratch and trying to enable everything needed trough the clock registers but most of them are access denied when accessed through /dev/mem or devmem2 
this is as far as i got and linux is not an OS/system/kernel i know well sadly.
linux knows tsec is there but accessing is not as direct as i had hoped. if anyone can help me out with that, i would love to add that functionality!
