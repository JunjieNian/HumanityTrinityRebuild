"""Check evaluated chair footprints, including shells, supports and casters."""
import re
import math
import bpy


def hull(points):
    points = sorted(set(points))
    def cross(a,b,c):
        return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    def chain(seq):
        result = []
        for p in seq:
            while len(result)>1 and cross(result[-2],result[-1],p)<=0:
                result.pop()
            result.append(p)
        return result
    return chain(points)[:-1]+chain(reversed(points))[:-1]


bpy.context.view_layer.update()
depsgraph = bpy.context.evaluated_depsgraph_get()
points = {}
for obj in bpy.context.scene.objects:
    match = re.match(r"(PhotoChair_\d+_\d+)_",obj.name)
    if not match or obj.type != "MESH":
        continue
    evaluated = obj.evaluated_get(depsgraph)
    mesh = evaluated.to_mesh()
    coords = points.setdefault(match[1],[])
    for vert in mesh.vertices:
        world = evaluated.matrix_world@vert.co
        coords.append((world.x,world.y))
    evaluated.to_mesh_clear()
footprints = {name:hull(coords) for name,coords in points.items()}
minimum, closest = math.inf, None
items = list(footprints.items())
for i,(name,a) in enumerate(items):
    for other,b in items[i+1:]:
        separation = -math.inf
        for polygon in (a,b):
            for p,q in zip(polygon,polygon[1:]+polygon[:1]):
                dx,dy=q[0]-p[0],q[1]-p[1]
                norm=math.hypot(dx,dy)
                axis=(-dy/norm,dx/norm)
                pa=[x*axis[0]+y*axis[1] for x,y in a]
                pb=[x*axis[0]+y*axis[1] for x,y in b]
                separation=max(separation,min(pb)-max(pa),min(pa)-max(pb))
        if separation<minimum:
            minimum,closest=separation,(name,other)
assert items and len(items)==globals().get("expected_chairs",len(items)), f"Unexpected chair count: {len(items)}"
assert minimum>.08, f"Chair clearance below 8 cm: {minimum:.4f} m {closest}"
print(f"CHAIR_CLEARANCE_PASS chairs={len(items)} conservative_gap_m={minimum:.4f} pair={closest}")
