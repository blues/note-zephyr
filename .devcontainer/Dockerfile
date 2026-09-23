# Pre-built development container for note-zephyr.
#
# The Zephyr west workspace lives at /workdir. At runtime the devcontainer only
# bind mounts /workdir/note-zephyr over this image, so everything else under
# /workdir (.west/, zephyr/, modules/) survives and does not need to be fetched
# when the container starts.

ARG ZEPHYR_BUILD_TAG=v0.26-branch
FROM zephyrprojectrtos/zephyr-build:${ZEPHYR_BUILD_TAG}

# The devcontainer runs as root, so build the workspace as root too; otherwise
# the CMake package registry and pip packages land in the wrong home directory.
USER root
WORKDIR /workdir

# Only the manifest is needed to populate the workspace, and copying it on its
# own keeps the Docker layer cache keyed on west.yml rather than on every commit.
COPY west.yml /workdir/note-zephyr/west.yml

# ZEPHYR_BASE is set by the base image, but /workdir/zephyr does not exist yet.
# zephyr-export is a Zephyr extension command, so Zephyr's Python requirements
# have to be installed before it can run.
RUN unset ZEPHYR_BASE \
    && west init -l /workdir/note-zephyr \
    && west update --narrow -o=--depth=1 \
    && pip install -r /workdir/zephyr/scripts/requirements.txt --root-user-action=ignore \
    && west zephyr-export

# Toolchain environment, exported at the image level so that non-interactive
# shells (CMake Tools, cortex-debug, VS Code tasks) inherit it too, rather than
# only the interactive shell that used to source ~/.bashrc.
# ZSDK_ARCH is read by .vscode/settings.json, so it has to be a real variable
# rather than something derived at shell start. The SDK names its sysroots after
# uname -m, so pass aarch64 when building an arm64 image.
ARG ZSDK_ARCH=x86_64
ENV ZSDK_ARCH=${ZSDK_ARCH}
ENV ZSDK_PATH=/opt/toolchains/zephyr-sdk-${ZSDK_VERSION}
ENV PATH=${ZSDK_PATH}/sysroots/${ZSDK_ARCH}-pokysdk-linux/usr/bin:${PATH}
RUN ln -sf "${ZSDK_PATH}/sysroots/${ZSDK_ARCH}-pokysdk-linux/usr/bin/openocd" /usr/bin/openocd

# The workspace is bind mounted from the host, so its ownership will not match
# the container user.
RUN git config --global --add safe.directory '*'

# west tab completion and the alias the previous onCreateCommand.sh set up.
RUN mkdir -p /etc/bash_completion.d \
    && west completion bash > /etc/bash_completion.d/west \
    && echo "alias ll='ls -lah'" >> /etc/bash.bashrc
