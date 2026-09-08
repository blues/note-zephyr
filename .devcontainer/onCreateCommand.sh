#!/bin/bash
set -e

# The image already ships a populated west workspace at /workdir (see
# .devcontainer/Dockerfile), so this only reconciles that workspace with the
# west.yml of the repository that is bind mounted over /workdir/note-zephyr.
# When the manifest is unchanged this is a handful of git fetches; when it has
# been edited it pulls in whatever actually changed.
west update --narrow -o=--depth=1
# zephyr-export is a Zephyr extension command, so install Zephyr's Python
# requirements first in case the manifest moved to a version that needs more.
pip install -r /workdir/zephyr/scripts/requirements.txt --root-user-action=ignore
west zephyr-export
