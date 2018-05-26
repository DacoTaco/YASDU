# YASDU
Yasdu ( **Y**et **A**nother **S**witch **D**umping **U**tility ) is a set of switch linux tool used to dump & access switch data.
i developed this after wanting to dump my NAND, TSEC fw and keys but having only a small SD around. i had tried other tools around but was not pleased as the data was wrong , to big and didn't provide hashes to verify the dump ( \*cough\* https://github.com/daanhenke/httpnand/issues \*cough\* )

there for , i made a python script to do it myself

...and now im just adding tools while im playing with nand :')

Current tools :

- D.a.n (decrypted access of nand) : a tool to access encrypted nand partitions like they are normal partitions using FUSE in linux. can work with dumps or the actual nand in linux on the switch

- R.n.d (Random Nand Decryptor) : a tool to decrypt encrypted partition dumps

- NandFileServer : python based webserver that allows the downloading/dumping of Nand & its partitions. future planned to allow browsing and dumping of certain files of the nand using D.a.n
