
import nuke

nodeMenu = nuke.menu("Nodes")
drawMenu = nodeMenu.findItem("Draw")
drawMenu.addCommand("Ramp2", "nuke.createNode('Ramp2')")

filterMenu = nodeMenu.findItem("Filter")

filterMenu.addCommand("Driven Dilate", "nuke.createNode('DrivenDilate')")


