"""Validate the exported mesh pack and optionally render a CPU-only facing contact sheet."""
import argparse
import math
import struct
import hashlib
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preview", type=Path)
    parser.add_argument("--building-preview", type=Path)
    parser.add_argument("--monster-preview", type=Path)
    parser.add_argument("--furniture-preview", type=Path)
    parser.add_argument("--hunting-preview", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    data = (root / "resource/runtime/school_models.bin").read_bytes()
    assert data[:8] == b"MGSRES03"
    manifest = json.loads((root / "resource/runtime/manifest.json").read_text(encoding="utf-8"))
    for relative, expected in manifest["source_sha256"].items():
        assert hashlib.sha256((root / "resource/assets" / relative).read_bytes()).hexdigest() == expected, relative
    checksum = 2166136261
    for byte in data[:-4]:
        checksum = ((checksum ^ byte) * 16777619) & 0xffffffff
    assert checksum == struct.unpack_from("<I", data, len(data) - 4)[0]
    count, poses, directions = struct.unpack_from("<III", data, 8)
    assert poses == 11 and directions == 8
    sockets = struct.unpack_from("<32f", data, 20)
    assert all(math.isfinite(v) for v in sockets)
    styles = list(struct.iter_unpack("<6f", data[148:364]))
    assert len(styles) == 9
    for width, depth, door_x, door_y, label, shadow in styles:
        assert all(math.isfinite(v) for v in (width, depth, door_x, door_y, label, shadow))
        assert 0 < width < 32 and 0 < depth < door_y < 32
        assert label > 0 and shadow > 0
    offset = 364
    meshes = []
    for index in range(count + poses * directions):
        vertices = struct.unpack_from("<I", data, offset)[0]
        offset += 4
        assert 0 < vertices <= 60000 and vertices % 3 == 0
        mesh = list(struct.iter_unpack("<6f", data[offset:offset + vertices * 24]))
        assert len(mesh) == vertices
        for x, y, r, g, b, a in mesh:
            assert all(math.isfinite(v) for v in (x, y, r, g, b, a))
            assert abs(x) < 2048 and abs(y) < 2048
            assert all(0 <= v <= 1 for v in (r, g, b, a))
        offset += vertices * 24
        meshes.append(mesh)
    assert offset == len(data) - 4
    assert len({tuple(meshes[count + direction]) for direction in range(8)}) == 8
    if args.preview:
        from PIL import Image, ImageDraw, ImageFont
        image = Image.new("RGB", (1600, 600), (28, 31, 48))
        draw = ImageDraw.Draw(image)
        font = ImageFont.truetype("C:/Windows/Fonts/arial.ttf", 15)
        for row, pose in enumerate((0, 2, 5)):
            for direction in range(8):
                mesh = meshes[count + pose * 8 + direction]
                cx, cy = direction * 200 + 100, row * 200 + 150
                for index in range(0, len(mesh), 3):
                    tri = mesh[index:index + 3]
                    color = tuple(int(v * 255) for v in tri[0][2:5])
                    draw.polygon([(cx + x * 1.65, cy + y * 1.65) for x, y, *_ in tri], fill=color)
                dx = math.cos(direction * math.pi / 4)
                dy = math.sin(direction * math.pi / 4)
                x, y = cx + (dx - dy) * 30, cy + 15 + (dx + dy) * 15
                draw.line((cx, cy + 15, x, y), fill=(110, 225, 190), width=2)
                draw.ellipse((x - 3, y - 3, x + 3, y + 3), fill=(110, 225, 190))
                if pose == 5:
                    x = cx + sockets[direction * 4 + 2] * 1.65
                    y = cy + sockets[direction * 4 + 3] * 1.65
                    draw.line((x - 4, y, x + 4, y), fill=(255, 90, 100), width=2)
                    draw.line((x, y - 4, x, y + 4), fill=(255, 90, 100), width=2)
                draw.text((direction * 200 + 30, row * 200 + 180),
                          f"{('Idle', 'Walk', 'Cast')[row]} / sector {direction}", fill="white", font=font)
        image.save(args.preview)
    if args.building_preview:
        from PIL import Image, ImageDraw
        names = (root / "SimpleGame/PrimitiveModels.h").read_text(encoding="utf-8")
        import re
        enum = re.search(r"enum class ModelId[^\{]*\{(.*?)\};", names, re.S).group(1)
        identifiers = re.findall(r"\b[A-Za-z_]\w*\b", re.sub(r"//[^\n]*", "", enum))
        selected = [(i, name) for i, name in enumerate(identifiers)
                    if name.startswith("Village") and name != "Count"]
        image = Image.new("RGB", (1800, 1200), (38, 43, 53))
        draw = ImageDraw.Draw(image)
        for cell, (model, name) in enumerate(selected):
            cx, cy = (cell % 3) * 600 + 300, (cell // 3) * 400 + 340
            mesh = meshes[model]
            for index in range(0, len(mesh), 3):
                tri = mesh[index:index + 3]
                draw.polygon([(cx + x, cy + y) for x, y, *_ in tri],
                             fill=tuple(int(v * 255) for v in tri[0][2:5]))
            draw.text((cx - 110, cy + 35), name, fill="white")
        image.save(args.building_preview)
    if args.monster_preview:
        from PIL import Image, ImageDraw
        import re
        header = (root / "SimpleGame/PrimitiveModels.h").read_text(encoding="utf-8")
        enum = re.search(r"enum class ModelId\s*\{(.*?)\bCount", header, re.S).group(1)
        ids = [name.strip() for name in enum.split(",") if name.strip()]
        image = Image.new("RGB", (1200, 1650), (30, 33, 45))
        draw = ImageDraw.Draw(image)
        for kind in range(11):
            for frame in range(6):
                mesh = meshes[ids.index(f"Mob{kind}Frame{frame + 4}")]
                cx, cy = frame * 200 + 100, kind * 150 + 120
                for index in range(0, len(mesh), 3):
                    tri = mesh[index:index + 3]
                    draw.polygon([(cx + x, cy + y) for x, y, *_ in tri],
                                 fill=tuple(int(v * 255) for v in tri[0][2:5]))
                draw.text((cx - 45, cy + 15), f"Mob {kind} / attack {frame}", fill="white")
        image.save(args.monster_preview)
    if args.furniture_preview:
        from PIL import Image, ImageDraw
        import re
        header = (root / "SimpleGame/PrimitiveModels.h").read_text(encoding="utf-8")
        enum = re.search(r"enum class ModelId\s*\{(.*?)\bCount", header, re.S).group(1)
        ids = [name.strip() for name in enum.split(",") if name.strip()]
        selected = [f"Interior{name}View{view}" for name in ("Bed", "StudyDesk", "Sofa", "Counter", "Wardrobe", "Mirror") for view in (0, 1)]
        image = Image.new("RGB", (1200, 750), (38, 42, 54))
        draw = ImageDraw.Draw(image)
        for cell, name in enumerate(selected):
            mesh = meshes[ids.index(name)]
            cx, cy = cell % 4 * 300 + 150, cell // 4 * 250 + 190
            for index in range(0, len(mesh), 3):
                tri = mesh[index:index + 3]
                draw.polygon([(cx + x * 2, cy + y * 2) for x, y, *_ in tri],
                             fill=tuple(int(v * 255) for v in tri[0][2:5]))
            draw.text((cx - 90, cy + 30), name, fill="white")
        image.save(args.furniture_preview)
    if args.hunting_preview:
        from PIL import Image, ImageDraw
        import re
        header = (root / "SimpleGame/PrimitiveModels.h").read_text(encoding="utf-8")
        enum = re.search(r"enum class ModelId\s*\{(.*?)\bCount", header, re.S).group(1)
        ids = [name.strip() for name in enum.split(",") if name.strip()]
        start = ids.index("MobFacingBegin")
        assert count == start + 960
        selected = [(f"kind {kind} / dir {direction}", start + kind * 80 + direction)
                    for kind in (0, 1, 3, 6, 11) for direction in range(8)]
        selected += [(name, ids.index(name)) for name in ("HealthPotion", "ManaPotion")]
        image = Image.new("RGB", (1280, 1020), (38, 42, 54))
        draw = ImageDraw.Draw(image)
        for cell, (name, index) in enumerate(selected):
            mesh = meshes[index]
            cx, cy = cell % 8 * 160 + 80, cell // 8 * 170 + 125
            for i in range(0, len(mesh), 3):
                tri = mesh[i:i + 3]
                draw.polygon([(cx + x * 1.8, cy + y * 1.8) for x, y, *_ in tri],
                             fill=tuple(int(v * 255) for v in tri[0][2:5]))
            draw.text((cx - 70, cy + 20), name, fill="white")
        image.save(args.hunting_preview)
    print(f"Resource pack passed: {count} base meshes, {poses * directions} facing poses, {len(data)} bytes")


if __name__ == "__main__":
    main()
