import os
from pathlib import Path

from selfdrive.hardware import PC

BASEDIR = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), "../"))

from selfdrive.hardware import PC
is_tbp = os.path.isfile('/data/tinkla_buddy_pro')
if PC:
  PERSIST = os.path.join(str(Path.home()), ".comma", "persist")
elif is_tbp:
  PERSIST = "/data/params/persist"
  PARAMS = "/data/params"
else:
  PERSIST = "/persist"
