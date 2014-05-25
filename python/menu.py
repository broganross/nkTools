
import nuke

nodeMenu = nuke.menu("Nodes")
filterMenu = nodeMenu.findItem("Filter")

filterMenu.addCommand("Driven Dilate", "nuke.createNode('DrivenDilate')")

