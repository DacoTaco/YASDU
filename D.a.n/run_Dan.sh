#!/bin/bash


if [ ! -d "/mnt/Dan" ]; then

	echo 'creating directory...'
	sudo mkdir /mnt/Dan
	sudo chmod 777 /mnt/Dan

fi

./Dan -f -o allow_root /mnt/Dan/