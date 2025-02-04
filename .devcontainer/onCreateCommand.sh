#!/bin/bash

unset ZEPHYR_BASE
west init -l .
west update
west zephyr-export
pip install -r /workdir/zephyr/scripts/requirements.txt --root-user-action
echo "alias ll='ls -lah'" >> $HOME/.bashrc
west completion bash > $HOME/west-completion.bash
echo 'source $HOME/west-completion.bash' >> $HOME/.bashrc
history -c
