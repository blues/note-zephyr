# Build development environment
# docker build --tag esp-dev-env .

# Launch development environment
# docker run --device /dev/bus/usb/ --interactive --rm --tty --volume "$(pwd)":/host-volume/ esp-dev-env

# _**NOTE:** In order to utilize DFU and debugging functionality, you must
# install (copy) the `.rules` file related to your debugging probe into the
# `/etc/udev/rules.d` directory of the host machine and restart the host._

# _**NOTE:** The debugging probe must be attached while the container is
# launched in order for it to be accessible from inside the container._

# Define global arguments
ARG DEBIAN_FRONTEND="noninteractive"

# Zephyr Build (base image)
FROM zephyrprojectrtos/zephyr-build:v0.26-branch

ENV ZSDK_PATH="/opt/toolchains/zephyr-sdk-${ZSDK_VERSION}"

RUN ["dash", "-c", "\
    mkdir /workdir/note-zephyr \
"]

COPY west.yml /workdir/note-zephyr/west.yml

WORKDIR /workdir/note-zephyr

# Initialize west environment
RUN ["dash", "-c", "\
    west init \
     --local . \
 && west update \
     --fetch smart \
     --narrow \
     --stats \
 && west zephyr-export \
 && pip install \
     --requirement ${ZEPHYR_BASE}/scripts/requirements.txt \
 && west completion bash > ${HOME}/west-completion.bash \
 && echo \"source ${HOME}/west-completion.bash\" >> ${HOME}/.bashrc \
"]
