#!/usr/bin/env sh
#
# Copyright (c) 2011 - 2015 ASPECTRON Inc.
# All Rights Reserved.
#
# This file is part of JSX (https://github.com/aspectron/jsx) project.
#
# Distributed under the MIT software license, see the accompanying
# file LICENSE
#

UNAME=`uname`

if [[ $UNAME == 'Linux' ]]; then

	apt-get -y install python build-essential

elif [[ $UNAME == 'Darwin' ]]; then
	sudo port install python

else

	echo "Unable to determine OS"

fi 
