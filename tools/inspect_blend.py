import bpy
import mathutils

print("=== MESH OBJECT DETAILS ===")
for obj in bpy.data.objects:
    if obj.type == 'MESH':
        bbox = [obj.matrix_world @ mathutils.Vector(corner) for corner in obj.bound_box]
        min_c = [min(v[i] for v in bbox) for i in range(3)]
        max_c = [max(v[i] for v in bbox) for i in range(3)]
        print(f"Object: {obj.name:20s} loc: {[round(c, 3) for c in obj.location]} "
              f"min: {[round(c, 3) for c in min_c]} max: {[round(c, 3) for c in max_c]}")
