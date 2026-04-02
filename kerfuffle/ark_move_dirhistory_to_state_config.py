# SPDX-FileCopyrightText: 2026 Lyosha Kirpel <lyoshakirpel@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause

import configparser
import os
import sys
from pathlib import Path


# configparser lowercases option names by default, but KConfig is case-sensitive.
# Subclassing is necessary to avoid a type error: optionxform is typed as a method
# with a `self` parameter, so directly assigning `instance.optionxform = str`
# causes a type mismatch in language servers (pyright/pylance).
class KConfigParser(configparser.ConfigParser):
    def optionxform(self, optionstr: str) -> str:
        return optionstr


SECTION = "ExtractDialog"
KEY = "DirHistory"

XDG_CONFIG_HOME = Path(os.getenv("XDG_CONFIG_HOME") or (Path.home() / ".config"))
XDG_STATE_HOME = Path(os.getenv("XDG_STATE_HOME") or (Path.home() / ".local/state"))

config_file = XDG_CONFIG_HOME / "arkrc"
state_config_file = XDG_STATE_HOME / "arkstaterc"

if not config_file.exists():
    sys.exit(0)

config = KConfigParser(delimiters=("="))
config.read(config_file)

if not config.has_option(SECTION, KEY):
    sys.exit(0)

dir_history = config.get(SECTION, KEY)

state_config = KConfigParser(delimiters=("="))
state_config.read(state_config_file)

if not state_config.has_section(SECTION):
    state_config.add_section(SECTION)

state_config.set(SECTION, KEY, dir_history)

state_config_file.parent.mkdir(parents=True, exist_ok=True)
with open(state_config_file, "w") as f:
    state_config.write(f, space_around_delimiters=False)

config.remove_option(SECTION, KEY)
if not config.options(SECTION):
    config.remove_section(SECTION)

with open(config_file, "w") as f:
    config.write(f, space_around_delimiters=False)
