#!/bin/bash

unset ZEPHYR_BASE
west init -l .
west update
west zephyr-export
pip install -r /workdir/zephyr/scripts/requirements.txt --root-user-action
echo "alias ll='ls -lah'" >> $HOME/.bashrc
west completion bash > $HOME/west-completion.bash
echo 'source $HOME/west-completion.bash' >> $HOME/.bashrc
echo "PATH=/opt/toolchains/zephyr-sdk-${ZSDK_VERSION}/sysroots/$(uname -m)-pokysdk-linux/usr/bin:\$PATH" >> $HOME/.bashrc
history -c
