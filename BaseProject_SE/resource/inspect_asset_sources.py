"""Read-only Blender inventory for updating the runtime exporter."""
import bpy
import json
from pathlib import Path

root = Path(__file__).resolve().parent / "assets"
for relative in ["characters/Hero_Magical.blend", "characters/Male_Student_A.blend",
                 "monsters/GhostMage.blend", "monsters/Grimoire.blend", "weapons/NightmareStaff_T1.blend", "buildings/School_Assembly.blend",
                 "buildings/House_Assembly.blend", "buildings/CoastalShop_Assembly.blend",
                 "buildings/Building_Details.blend", "buildings/Construction_Modules.blend",
                 "props/Furniture_Props.blend", "nature/Nature_Modules.blend",
                 "nature/Tree_A.blend", "nature/Grass_A.blend", "weapons/StarWand_Base.blend"]:
    bpy.ops.wm.open_mainfile(filepath=str(root / relative))
    for collection in bpy.data.collections:
        if collection.name not in bpy.context.scene.collection.children:
            bpy.context.scene.collection.children.link(collection)
    bpy.context.view_layer.update()
    print("INVENTORY", json.dumps({"file": relative,
        "collections": [c.name for c in bpy.data.collections],
        "rigs": [{"name": o.name, "position": list(o.location),
                  "children": [c.name for c in o.children],
                  "actions": [t.name for t in o.animation_data.nla_tracks] if o.animation_data else []}
                 for o in bpy.data.objects if o.type == "ARMATURE"],
        "objects": [{"name": o.name, "type": o.type, "position": list(o.location),
                     "dimensions": list(o.dimensions), "properties": dict(o.items())}
                    for o in bpy.data.objects if o.type != "ARMATURE"]}, default=str))
