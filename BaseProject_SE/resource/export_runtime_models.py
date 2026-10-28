"""Bake resource/assets libraries into the existing quarter-view renderer's mesh pack.

Blender --background --python resource/export_runtime_models.py
Only resource/runtime outputs are written. Source .blend files remain unchanged.
"""
import hashlib
import json
import math
import re
import struct
from pathlib import Path

import bpy
from mathutils import Matrix, Vector

ROOT = Path(__file__).resolve().parent
ASSETS = ROOT / "assets"
REPORT = {}
SOURCES = {}
STATIC = {}
POSES = {}


def open_source(relative):
    path = ASSETS / relative
    if not path.is_file():
        raise FileNotFoundError(path)
    SOURCES[relative] = hashlib.sha256(path.read_bytes()).hexdigest()
    bpy.ops.wm.open_mainfile(filepath=str(path))
    # These are data libraries rather than saved scenes: link their collections for evaluation.
    for collection in list(bpy.data.collections):
        if collection.name not in bpy.context.scene.collection.children:
            bpy.context.scene.collection.children.link(collection)
    bpy.context.view_layer.update()


def geometry(objects, origin):
    result = []
    deps = bpy.context.evaluated_depsgraph_get()
    for obj in objects:
        if obj.type != "MESH":
            continue
        evaluated = obj.evaluated_get(deps)
        mesh = evaluated.to_mesh()
        mesh.calc_loop_triangles()
        for tri in mesh.loop_triangles:
            material = mesh.materials[tri.material_index] if mesh.materials else None
            color = tuple(material.diffuse_color) if material else (.8, .8, .8, 1)
            vertices = [evaluated.matrix_world @ mesh.vertices[i].co - origin for i in tri.vertices]
            result.append((vertices, color))
        evaluated.to_mesh_clear()
    return result


def modify(mesh, scale=(1, 1, 1), offset=(0, 0, 0), tint=(1, 1, 1), rotation=0):
    matrix = Matrix.Rotation(rotation, 3, "Z")
    return [([matrix @ Vector((v.x * scale[0], v.y * scale[1], v.z * scale[2])) + Vector(offset)
              for v in item[0]], tuple(min(1, item[1][i] * tint[i]) for i in range(3)) + (item[1][3],),
             *item[2:]) for item in mesh]


def centered(mesh):
    points = [v for item in mesh for v in item[0]]
    origin = Vector(((min(v.x for v in points) + max(v.x for v in points)) / 2,
                     (min(v.y for v in points) + max(v.y for v in points)) / 2,
                     min(v.z for v in points)))
    return modify(mesh, offset=-origin)


def static_source(relative):
    if relative not in STATIC:
        open_source(relative)
        STATIC[relative] = {obj.name: geometry([obj], Vector())
                            for obj in bpy.data.objects if obj.type == "MESH"}
    return STATIC[relative]


def select(relative, predicate):
    result = [tri for name, mesh in static_source(relative).items() if predicate(name) for tri in mesh]
    if not result:
        raise ValueError("No source meshes matched: " + relative)
    return result


def module(prefix):
    if prefix.startswith("Terrain_"):
        relative = "nature/Nature_Modules.blend"
    elif prefix in ("Floor_Module", "Roof_Module", "Door_Panel", "Wall_Plaster", "Window_Plaster"):
        relative = "buildings/Construction_Modules.blend"
    else:
        relative = "props/Furniture_Props.blend"
    return centered(select(relative, lambda n: n == prefix or n.startswith(prefix + "_")))


def character_raw(name, clip="Idle", frame=1):
    key = name, clip, frame
    if key not in POSES:
        folder = "characters" if (ASSETS / "characters" / (name + ".blend")).is_file() else "monsters"
        open_source(folder + "/" + name + ".blend")
        rig = next(obj for obj in bpy.data.objects if obj.type == "ARMATURE")
        track = next(t for t in rig.animation_data.nla_tracks if t.name == clip)
        rig.animation_data.action = track.strips[0].action
        bpy.context.scene.frame_set(frame)
        bpy.context.view_layer.update()
        origin = rig.matrix_world.translation
        raw = geometry(rig.children, origin)
        socket = None
        if name == "Hero_Magical":
            star = next(o for o in rig.children if "__Equipped_StarWand__Star" in o.name)
            points = [v for tri, _ in geometry([star], origin) for v in tri]
            socket = Vector(tuple((min(v[i] for v in points) + max(v[i] for v in points)) / 2
                                  for i in range(3)))
        POSES[key] = raw, socket
    return POSES[key]


def character(name, clip="Idle", frame=1, angle=math.pi / 2):
    raw, point = character_raw(name, clip, frame)
    socket = None
    if point is not None:
        point = Matrix.Rotation(angle, 3, "Z") @ point
        socket = ((point.x - point.y) * 28, (point.x + point.y) * 14 - point.z * 28)
    return modify(raw, rotation=angle), socket


def visibility_order(mesh):
    """Offline BSP: split crossing faces before painter ordering orthographic geometry."""
    polygons = []
    view = Vector((1, 1, 1))
    for item in mesh:
        vertices, color = item[:2]
        normal = (vertices[1] - vertices[0]).cross(vertices[2] - vertices[0])
        if normal.length > 1e-8 and normal.dot(view) > 0:
            polygons.append((vertices, color))

    def order(items):
        if not items:
            return []
        points = items[len(items) // 2][0]
        normal = (points[1] - points[0]).cross(points[2] - points[0]).normalized()
        distance = normal.dot(points[0])
        behind, coplanar, front = [], [], []
        for vertices, color in items:
            distances = [normal.dot(v) - distance for v in vertices]
            if max(distances) <= 1e-5 and min(distances) >= -1e-5:
                coplanar.append((vertices, color))
            elif max(distances) <= 1e-5:
                behind.append((vertices, color))
            elif min(distances) >= -1e-5:
                front.append((vertices, color))
            else:
                negative, positive = [], []
                for i, a in enumerate(vertices):
                    b = vertices[(i + 1) % len(vertices)]
                    da, db = distances[i], distances[(i + 1) % len(vertices)]
                    if da <= 1e-5:
                        negative.append(a)
                    if da >= -1e-5:
                        positive.append(a)
                    if (da < -1e-5 and db > 1e-5) or (da > 1e-5 and db < -1e-5):
                        crossing = a + (b - a) * (da / (da - db))
                        negative.append(crossing)
                        positive.append(crossing)
                if len(negative) >= 3:
                    behind.append((negative, color))
                if len(positive) >= 3:
                    front.append((positive, color))
        return order(behind) + coplanar + order(front)

    return [([points[0], points[i], points[i + 1]], color)
            for points, color in order(polygons) for i in range(1, len(points) - 1)]


def bake(mesh, pixels=32, ordered=False):
    triangles = []
    light = Vector((-.3, -.5, 1)).normalized()
    for item in mesh:
        vertices, color = item[:2]
        layer = item[2] if len(item) > 2 else 0
        normal = (vertices[1] - vertices[0]).cross(vertices[2] - vertices[0])
        if normal.length < 1e-8:
            continue
        normal.normalize()
        if normal.dot(Vector((1, 1, 1))) <= 0:
            continue
        shade = .72 + .28 * max(0, normal.dot(light))
        rgb = [min(1, max(0, color[i] * shade)) for i in range(3)]
        depth = sum(v.x + v.y + v.z for v in vertices) / 3
        vertices_2d = [(float((v.x - v.y) * pixels),
                        float((v.x + v.y) * pixels * .5 - v.z * pixels), *rgb, color[3])
                       for v in vertices]
        triangles.append(((layer, depth), vertices_2d))
    if not ordered:
        triangles.sort(key=lambda t: t[0])
    return [vertex for _, tri in triangles for vertex in tri]


def tagged(mesh, layer):
    return [(item[0], item[1], layer) for item in mesh]


def double_sided(mesh):
    return mesh + [(list(reversed(item[0])), item[1], *item[2:]) for item in mesh]


meshes = {}
for prefix, source in [("Player", "Hero_Magical"), ("Student", "Hero_Student")]:
    meshes[prefix + "Idle"] = bake(character(source)[0], 28)
    for frame in range(4):
        meshes[prefix + "Walk" + str(frame)] = bake(character(source, "Walk", 1 + frame * 7)[0], 28)
meshes["PlayerCast"] = bake(character("Hero_Magical", "Attack", 13)[0], 28)
for prefix, source in [("Slime", "Slime"), ("Shade", "GhostMage")]:
    for frame in range(4):
        meshes[prefix + str(frame)] = bake(character(source, "Walk", 1 + frame * 7)[0], 28)
for target, source in [("Yuna", "Yuna"), ("Jisoo", "Jisoo"), ("Lumi", "Lumi"),
                       ("ShadowEater", "ShadowEater_Boss"), ("ShadowRabbit", "ShadowRabbit"),
                       ("PoisonMushroom", "PoisonMushroom"), ("Grimoire", "Grimoire"),
                       ("SchoolGuard", "Male_Student_A"), ("StoreClerk", "Male_Student_B")]:
    meshes[target] = bake(character(source)[0], 28)
meshes["YunaFallen"] = [(float(-y * .85 - 24), float(x * .65 - 5), r, g, b, a)
                       for x, y, r, g, b, a in meshes["Yuna"]]

for target, source, pixels in [
    ("FloorWood", "Floor_Module", 16), ("FloorStone", "Terrain_Pavement", 16),
    ("TerrainGrass", "Terrain_Grass", 16), ("TerrainSand", "Terrain_Sand", 16),
    ("TerrainWater", "Terrain_Water", 16), ("Wall", "Wall_Plaster", 32),
    ("Window", "Window_Plaster", 32), ("Door", "Door_Panel", 32),
    ("Desk", "Desk", 32), ("Chair", "Chair", 32), ("Bed", "Bed", 32),
    ("Bench", "Bench", 32), ("StreetLamp", "StreetLamp", 32)]:
    meshes[target] = bake(module(source), pixels)

cabinet = centered(select("buildings/Building_Details.blend",
    lambda n: n in ("Cabinet_Case_0", "Cabinet_Door_0") or n.startswith("Cabinet_Handle_0_")))
cabinet = modify(cabinet, rotation=math.pi)
meshes["Cabinet"] = bake(cabinet)
meshes["Fence"] = bake(centered(select("buildings/Building_Details.blend",
                                      lambda n: n.startswith("SchoolFence_"))))
book = centered(character_raw("Grimoire")[0])
book = modify(book, scale=(.45, .6, .45), rotation=math.pi)
meshes["Book"] = bake(book, 16)
meshes["YunaBag"] = bake(modify(module("Bed"), scale=(.35, .7, .75)), 18)
meshes["Blackboard"] = bake(modify(module("Door_Panel"), scale=(2.5, .5, .55),
                                  tint=(.18, .44, .29)), 32)
shelf = tagged(cabinet, 0)
for z in (.35, .95, 1.55):
    for x in (-.4, -.13, .13, .4):
        shelf += tagged(modify(book, scale=(.4, .45, .55), offset=(x, .43, z),
                               tint=(1, .85 + x / 4, .8)), 1)
meshes["Bookshelf"] = bake(shelf)
planter = modify(module("Bench"), scale=(.55, .8, .55))
grass = double_sided(centered(select("nature/Grass_A.blend", lambda n: True)))
planter += modify(grass, scale=(.65, .65, .7), offset=(0, 0, .43))
meshes["Planter"] = bake(planter)
beacon = planter + modify(centered(select("weapons/NightmareStaff_T1.blend",
                        lambda n: "Crystal" in n)), offset=(0, 0, .7))
meshes["Campfire"] = bake(beacon)
for target, file, part in [("Coin", "StarWand_Base", "__Star"),
                           ("Crystal", "NightmareStaff_T1", "__Crystal"),
                           ("BlackFragment", "NightmareStaff_T1", "__Crystal"),
                           ("Potion", "NightmareStaff_T1", "__Crystal")]:
    raw = centered(select("weapons/" + file + ".blend", lambda n: part in n))
    if target == "BlackFragment":
        raw = modify(raw, tint=(.35, .24, .55))
    meshes[target] = bake(raw, 22 if target == "Potion" else 32)

for category, names in [("Tree", ["Tree", "TreeB", "TreeC"]),
                        ("Rock", ["Rock", "RockB", "RockC"]),
                        ("Grass", ["GrassA", "GrassB", "GrassC"])]:
    for variant, target in enumerate(names):
        relative = "nature/" + category + "_" + "ABC"[variant] + ".blend"
        raw = select(relative, lambda n: True)
        if category == "Grass":
            raw = double_sided(raw)
        meshes[target] = bake(raw, 32)

building_info = []
roof_colors = [( .68, .58, .88), (.92, .62, .70), (.48, .68, .84), (.82, .75, .52),
               (.43, .50, .68), (.88, .58, .67), (.55, .67, .79), (.60, .78, .73), (.66, .57, .84)]


def box_mesh(center, size, color):
    vertices = [Vector((center[0] + x * size[0] / 2,
                        center[1] + y * size[1] / 2,
                        center[2] + z * size[2] / 2))
                for z in (-1, 1) for y in (-1, 1) for x in (-1, 1)]
    faces = [(0, 2, 3, 1), (4, 5, 7, 6), (0, 1, 5, 4),
             (2, 6, 7, 3), (0, 4, 6, 2), (1, 3, 7, 5)]
    return [([vertices[a], vertices[b], vertices[c]], (*color, 1))
            for a, b, c, d in faces for a, b, c in ((a, b, c), (a, c, d))]


def building(variant):
    if variant == 8:
        # An open park, with a clear central promenade rather than a solid building.
        raw = []
        bench = module("Bench")
        for x in (-2.4, 2.4):
            raw += modify(bench, scale=(.7, .7, .7), offset=(x, 0, 0))
        tree = select("nature/Tree_B.blend", lambda n: True)
        for x in (-2.7, 2.7):
            raw += modify(tree, scale=(.65, .65, .65), offset=(x, -2, 0))
        baked = bake(raw)
        building_info.append([3.5, 2.5, 0, 3.2, -min(v[1] for v in baked) + 20, 70])
        return baked
    school = variant == 4
    shop = variant in (3, 5, 7)
    name = "School_Assembly" if school else "CoastalShop_Assembly" if shop else "House_Assembly"
    objects = dict(static_source("buildings/" + name + ".blend"))
    foundation = objects[name + "__Foundation"]
    points = [v for tri, _ in foundation for v in tri]
    origin = Vector(((min(v.x for v in points) + max(v.x for v in points)) / 2,
                     (min(v.y for v in points) + max(v.y for v in points)) / 2,
                     min(v.z for v in points)))
    scale = (1, 1, 1) if school or shop else (.9 + variant % 3 * .04, .95, .78 + variant % 3 * .08)
    if variant == 1:
        scale = (1.15, 1.05, .85)
    elif variant == 2:
        scale = (1.5, 1.15, .65)
    elif variant == 3:
        scale = (1.2, .9, .65)
    if school:
        objects.update({n: m for n, m in static_source("buildings/Building_Details.blend").items()
                        if n.startswith("SchoolClock_")})
    result = []
    for part, mesh in objects.items():
        if "Door" in part:
            continue
        layer = 0 if "Foundation" in part else 1 if "__Wall" in part else 2
        if "WindowFrame" in part:
            layer = 3
        elif "__Glass" in part:
            layer = 4
        elif any(token in part for token in ("Mullion", "Sill", "Clock")):
            layer = 5
        elif any(token in part for token in ("Door", "Canopy", "EntryStep")):
            layer = 6
        elif "__Roof" in part:
            layer = 7
        adjusted = modify(mesh, offset=-origin)
        if "__Wall" in part:
            palette = [(1, .90, .80), (.88, .97, .96), (.96, .88, .96),
                       (.76, .86, .91), (.96, .93, .85), (.99, .88, .78),
                       (.87, .91, .99), (.91, .98, .91), (.94, .88, .98)]
            adjusted = [(tri, (*palette[variant], c[3])) for tri, c in adjusted]
        if "__Roof" in part or "__Canopy" in part:
            # Preserve the authored facade/glass; adjust only the roof family.
            color = roof_colors[variant]
            adjusted = [(tri, (*color, c[3])) for tri, c in adjusted]
        adjusted = modify(adjusted, scale=scale, rotation=math.pi)
        result += tagged(adjusted, layer)
    # Solid exterior shell covers internal floor slabs; facade details remain above it.
    half_x = (max(v.x for v in points) - min(v.x for v in points)) * scale[0] / 2
    half_y = (max(v.y for v in points) - min(v.y for v in points)) * scale[1] / 2
    shell_height = (3.0 if shop else 5.45) * scale[2]
    wall_color = [(1, .90, .80), (.88, .97, .96), (.96, .88, .96),
                  (.76, .86, .91), (.96, .93, .85), (.99, .88, .78),
                  (.87, .91, .99), (.91, .98, .91), (.94, .88, .98)][variant]
    result += tagged(box_mesh((0, 0, shell_height / 2),
                             (half_x * 2 - .16, half_y * 2 - .16, shell_height), wall_color), 2.5)
    door_y = half_y + .03
    result += tagged(box_mesh((0, door_y, .70), (1.05, .13, 1.4), (.28, .35, .43)), 6)
    result += tagged(box_mesh((0, door_y + .09, .76), (.83, .06, 1.06), (.40, .71, .78)), 6.1)
    result += tagged(box_mesh((.31, door_y + .15, .65), (.06, .07, .22), (.94, .85, .53)), 6.2)
    # Purpose-specific signs: mint hospital cross, mall fascia, blue station entrance.
    facade_y = 1.72 * scale[1]
    if variant == 1:
        result += tagged(box_mesh((0, facade_y, 3.5), (1.3, .12, 1.3), (.98, .99, 1)), 8)
        result += tagged(box_mesh((0, facade_y + .09, 3.5), (.25, .1, .9), (.1, .65, .5)), 9)
        result += tagged(box_mesh((0, facade_y + .09, 3.5), (.9, .1, .25), (.1, .65, .5)), 9)
    elif variant in (2, 3):
        color = (.65, .36, .65) if variant == 2 else (.15, .44, .70)
        result += tagged(box_mesh((0, facade_y, 1.8), (3.3, .15, .48), color), 8)
        for x in (-1, 0, 1):
            result += tagged(box_mesh((x, facade_y + .12, 1.8), (.45, .08, .14), (.96, .97, .89)), 9)
    baked = bake(result)
    fx = (max(v.x for v in points) - min(v.x for v in points)) * scale[0] * .5
    fy = (max(v.y for v in points) - min(v.y for v in points)) * scale[1] * .5
    step_points = [v for n, m in objects.items() if "EntryStep" in n for tri, _ in m for v in tri]
    entry_y = max(-(v.y - origin.y) * scale[1] for v in step_points) + .22
    building_info.append([fx + .22, fy + .22, 0, entry_y,
                          -min(v[1] for v in baked) + 20, fx * 25])
    return baked


wood = (.53, .34, .23)
metal = (.20, .25, .29)
desk = box_mesh((0, 0, .8), (1.5, .8, .12), wood)
for x in (-.62, .62):
    for y in (-.28, .28):
        desk += box_mesh((x, y, .38), (.09, .09, .76), metal)
meshes["Desk"] = bake(desk)
bench = box_mesh((0, 0, .5), (1.8, .55, .12), wood)
bench += box_mesh((0, -.27, .9), (1.8, .10, .55), wood)
for x in (-.7, .7):
    bench += box_mesh((x, 0, .24), (.13, .5, .48), metal)
meshes["Bench"] = bake(bench)
bed = box_mesh((0, 0, .3), (1.3, 2, .6), wood)
bed += box_mesh((0, 0, .68), (1.25, 1.95, .22), (.95, .94, .90))
bed += box_mesh((0, .25, .83), (1.27, 1.35, .12), (.62, .58, .83))
bed += box_mesh((0, -.7, .86), (.9, .4, .15), (1, .98, .92))
bed += box_mesh((0, -1, .65), (1.4, .12, 1.3), wood)
meshes["Bed"] = bake(bed)
tv = box_mesh((0, 0, .45), (1.8, .6, .9), wood)
tv += box_mesh((0, 0, .96), (.65, .4, .08), metal)
tv += box_mesh((0, 0, 1.15), (.14, .12, .4), metal)
tv += box_mesh((0, 0, 1.55), (1.65, .16, .95), (.08, .1, .13))
tv = tagged(tv, 0) + tagged(box_mesh((0, .09, 1.55), (1.46, .02, .77), (.25, .48, .65)), 1)
meshes["Television"] = bake(tv)
for target, source in [("Yuna", "Yuna"), ("Jisoo", "Jisoo"),
                       ("SchoolGuard", "Male_Student_A"), ("StoreClerk", "Male_Student_B")]:
    for sector in range(8):
        meshes[target + "Facing" + str(sector)] = bake(character(
            source, angle=sector * math.pi / 4 + math.pi / 2)[0], 28)

for variant, name in enumerate(["VillageHouseLilac", "VillageHousePink", "VillageHouseBlue", "VillageHouseYellow",
                                 "VillageSchool", "VillageStore", "VillageAcademy", "VillageClinic", "VillageArtsAcademy"]):
    meshes[name] = building(variant)
meshes["Shelter"] = meshes["VillageSchool"]
meshes["Shadow"] = []
for i in range(20):
    a, b = i * math.tau / 20, (i + 1) * math.tau / 20
    for x, y in [(0, 0), (16 * math.cos(a), 6 * math.sin(a)), (16 * math.cos(b), 6 * math.sin(b))]:
        meshes["Shadow"].append((x, y, .02, .02, .04, .22))

catalog = json.loads((ASSETS / "characters/npcs/npc_catalog.json").read_text(encoding="utf-8"))
for entry in catalog["npcs"]:
    name = entry["id"]
    open_source("characters/npcs/" + name + ".blend")
    rig = next(o for o in bpy.data.objects if o.type == "ARMATURE")
    rig.animation_data.action = next(t for t in rig.animation_data.nla_tracks if t.name == "Idle").strips[0].action
    bpy.context.scene.frame_set(1)
    bpy.context.view_layer.update()
    raw = geometry(rig.children, rig.matrix_world.translation)
    for sector in range(8):
        meshes[name + "View" + str(sector)] = bake(modify(raw, rotation=sector * math.pi / 4 + math.pi / 2), 28)
for path in sorted((ASSETS / "interiors/furniture").glob("*.blend")):
    raw = select("interiors/furniture/" + path.name, lambda n: True)
    for sector in range(4):
        oriented = modify(raw, rotation=sector * math.pi / 2)
        meshes["Interior" + path.stem + "View" + str(sector)] = bake(visibility_order(oriented), 32, ordered=True)

monster_sources = ["Slime", "GhostMage", "PoisonMushroom", "ShadowRabbit", "Grimoire",
                   "NightmareBat", "CoralCrab", "MimicChest", "LanternWisp", "ThornSeed", "ShadowEater_Boss",
                   "Gym_EnhancedShadow_Boss"]
for kind, source in enumerate(monster_sources):
    for frame, sample in enumerate((1, 8, 16, 24)):
        meshes[f"Mob{kind}Frame{frame}"] = bake(character(source, "Walk", sample)[0], 28)
    for frame, sample in enumerate((1, 5, 9, 13, 19, 25)):
        meshes[f"Mob{kind}Frame{frame + 4}"] = bake(character(source, "Attack", sample)[0], 28)
    samples = [("Walk", f) for f in (1, 8, 16, 24)] + [("Attack", f) for f in (1, 5, 9, 13, 19, 25)]
    for frame, (clip, sample) in enumerate(samples):
        for sector in range(8):
            meshes[f"MobFacing{kind}_{frame}_{sector}"] = bake(
                character(source, clip, sample, sector * math.pi / 4 + math.pi / 2)[0], 28)


def potion_mesh(color):
    triangles = []
    rings = [(0, .18), (.08, .26), (.55, .26), (.68, .12), (.86, .12), (.92, .15)]
    for j, ((z0, r0), (z1, r1)) in enumerate(zip(rings, rings[1:])):
        for i in range(12):
            a, b = i * math.tau / 12, (i + 1) * math.tau / 12
            points = [Vector((r0 * math.cos(a), r0 * math.sin(a), z0)),
                      Vector((r0 * math.cos(b), r0 * math.sin(b), z0)),
                      Vector((r1 * math.cos(b), r1 * math.sin(b), z1)),
                      Vector((r1 * math.cos(a), r1 * math.sin(a), z1))]
            tint = (.94, .88, .69, 1) if j == 4 else tuple(color) + (1,)
            triangles.extend([(points[:3], tint), ([points[0], points[2], points[3]], tint)])
    top = [Vector((.15 * math.cos(i * math.tau / 12), .15 * math.sin(i * math.tau / 12), .92)) for i in range(12)]
    for i in range(12):
        triangles.append(([Vector((0, 0, .92)), top[i], top[(i + 1) % 12]], (.96, .9, .74, 1)))
    return bake(triangles, 30)


meshes["HealthPotion"] = potion_mesh((.95, .22, .35))
meshes["ManaPotion"] = potion_mesh((.25, .52, 1))

header = (ROOT.parent / "SimpleGame/PrimitiveModels.h").read_text(encoding="utf-8")
names = re.search(r"enum class ModelId\s*\{(.*?)\bCount", header, re.S).group(1)
names = [name.strip() for name in names.split(",") if name.strip()]
names.remove("MobFacingBegin")
names += [f"MobFacing{k}_{f}_{d}" for k in range(12) for f in range(10) for d in range(8)]
directional = []
direction_sockets = []
for sector in range(8):
    angle = sector * math.pi / 4 + math.pi / 2
    direction_sockets.extend(character("Hero_Magical", angle=angle)[1])
    direction_sockets.extend(character("Hero_Magical", "Attack", 13, angle)[1])
for source, clip, frame in [("Hero_Magical", "Idle", 1)] + [
        ("Hero_Magical", "Walk", 1 + i * 7) for i in range(4)] + [
        ("Hero_Magical", "Attack", 13), ("Hero_Student", "Idle", 1)] + [
        ("Hero_Student", "Walk", 1 + i * 7) for i in range(4)]:
    for sector in range(8):
        directional.append(bake(character(source, clip, frame, sector * math.pi / 4 + math.pi / 2)[0], 28))
assert len(direction_sockets) == 32 and all(math.isfinite(v) for v in direction_sockets)
payload = bytearray(b"MGSRES03" + struct.pack("<III32f", len(names), 11, 8, *direction_sockets))
for info in building_info:
    payload.extend(struct.pack("<6f", *info))
for name, mesh in [(name, meshes[name]) for name in names] + [
        ("facing_" + str(i), mesh) for i, mesh in enumerate(directional)]:
    assert mesh and len(mesh) % 3 == 0 and len(mesh) <= 60000, name
    assert all(math.isfinite(value) for vertex in mesh for value in vertex), name
    payload.extend(struct.pack("<I", len(mesh)))
    for vertex in mesh:
        payload.extend(struct.pack("<6f", *vertex))
    REPORT[name] = {"triangles": len(mesh) // 3}
checksum = 2166136261
for byte in payload:
    checksum = ((checksum ^ byte) * 16777619) & 0xffffffff
payload.extend(struct.pack("<I", checksum))
out = ROOT / "runtime"
out.mkdir(exist_ok=True)
temporary = out / "school_models.bin.tmp"
temporary.write_bytes(payload)
temporary.replace(out / "school_models.bin")
(out / "manifest.json").write_text(json.dumps({"source": "assets", "source_sha256": SOURCES,
    "format": "MGSRES03", "model_count": len(names), "facing_poses": 11, "facing_directions": 8,
    "wand_sockets_pixels": direction_sockets, "building_layout": building_info,
    "models": REPORT, "bytes": len(payload)}, ensure_ascii=False, indent=2), encoding="utf-8")
print("EXPORTED", len(names), "models + 88 directional poses,", len(payload), "bytes;", len(SOURCES), "source files")
