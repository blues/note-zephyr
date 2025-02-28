#!/bin/bash

unset ZEPHYR_BASE
west init -l .
west update
west zephyr-export
pip install -r /workdir/zephyr/scripts/requirements.txt --root-user-action
echo "alias ll='ls -lah'" >> $HOME/.bashrc
west completion bash > $HOME/west-completion.bash
echo 'source $HOME/west-completion.bash' >> $HOME/.bashrc
ZSDK_ARCH=$(uname -m)
echo "export ZSDK_ARCH=${ZSDK_ARCH}" >> $HOME/.bashrc
ZSDK_PATH="/opt/toolchains/zephyr-sdk-${ZSDK_VERSION}"
echo "export ZSDK_PATH=${ZSDK_PATH}" >> $HOME/.bashrc
echo "PATH=${ZSDK_PATH}/sysroots/${ZSDK_ARCH}-pokysdk-linux/usr/bin:\$PATH" >> $HOME/.bashrc
ln -s /opt/toolchains/zephyr-sdk-${ZSDK_VERSION}/sysroots/${ZSDK_ARCH}-pokysdk-linux/usr/bin/openocd /usr/bin/openocd
history -c
