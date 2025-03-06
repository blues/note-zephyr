#!/bin/bash

echo "export PATH=${ZSDK_PATH}/sysroots/$(uname -m)-pokysdk-linux/usr/bin:\$PATH" >> $HOME/.bashrc
history -c
