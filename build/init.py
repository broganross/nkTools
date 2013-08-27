import sys
import nuke

if sys.platform.startswith("win"):
	nuke.pluginAddPath("./win")
else:
	nuke.pluginAddPath("./nix")
