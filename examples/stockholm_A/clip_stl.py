import sys
import numpy as np
import struct

EPS = 1e-9

# ---------------- STL IO ----------------

def load_stl(path):
    with open(path, 'rb') as f:
        header = f.read(80)
        try:
            tri_count = struct.unpack('<I', f.read(4))[0]
            faces = []

            for _ in range(tri_count):
                data = f.read(50)
                if len(data) < 50:
                    break

                v1 = struct.unpack('<3f', data[12:24])
                v2 = struct.unpack('<3f', data[24:36])
                v3 = struct.unpack('<3f', data[36:48])

                faces.append([v1, v2, v3])

            return np.array(faces, dtype=np.float64)

        except:
            # fallback ASCII STL
            return load_ascii_stl(path)


def load_ascii_stl(path):
    faces = []
    with open(path, 'r') as f:
        tri = []
        for line in f:
            if line.strip().startswith('vertex'):
                tri.append(list(map(float, line.split()[1:])))
                if len(tri) == 3:
                    faces.append(tri)
                    tri = []
    return np.array(faces, dtype=np.float64)


def save_stl(path, triangles):
    with open(path, 'wb') as f:
        f.write(b'Python STL Writer' + b' ' * (80 - len('Python STL Writer')))
        f.write(struct.pack('<I', len(triangles)))

        for tri in triangles:
            v1, v2, v3 = tri

            normal = np.cross(v2 - v1, v3 - v1)
            norm = np.linalg.norm(normal)
            if norm > EPS:
                normal /= norm
            else:
                normal = np.array([0.0, 0.0, 0.0])

            f.write(struct.pack('<3f', *normal))
            f.write(struct.pack('<3f', *v1))
            f.write(struct.pack('<3f', *v2))
            f.write(struct.pack('<3f', *v3))
            f.write(struct.pack('<H', 0))


# ---------------- Clipping ----------------

def clip_polygon_against_plane(poly, axis, value, keep_less):
    new_poly = []

    def inside(p):
        return p[axis] <= value + EPS if keep_less else p[axis] >= value - EPS

    for i in range(len(poly)):
        curr = np.asarray(poly[i])
        prev = np.asarray(poly[i - 1])

        curr_in = inside(curr)
        prev_in = inside(prev)

        if curr_in:
            if not prev_in:
                denom = curr[axis] - prev[axis]
                if abs(denom) > EPS:
                    t = (value - prev[axis]) / denom
                    inter = prev + t * (curr - prev)
                    new_poly.append(inter)
            new_poly.append(curr)

        elif prev_in:
            denom = curr[axis] - prev[axis]
            if abs(denom) > EPS:
                t = (value - prev[axis]) / denom
                inter = prev + t * (curr - prev)
                new_poly.append(inter)

    return new_poly


def clip_triangle(tri, bmin, bmax):
    poly = [np.asarray(v) for v in tri]

    for axis in range(3):
        poly = clip_polygon_against_plane(poly, axis, bmin[axis], keep_less=False)
        if not poly:
            return []

        poly = clip_polygon_against_plane(poly, axis, bmax[axis], keep_less=True)
        if not poly:
            return []

    return poly


def triangulate(poly):
    tris = []
    for i in range(1, len(poly) - 1):
        tris.append([poly[0], poly[i], poly[i+1]])
    return tris


# ---------------- Main ----------------

def main():
    if len(sys.argv) != 8:
        print("Usage: python clip_stl input.stl xmin ymin zmin xmax ymax zmax")
        return

    input_file = sys.argv[1]
    xmin, ymin, zmin = map(float, sys.argv[2:5])
    xmax, ymax, zmax = map(float, sys.argv[5:8])

    bmin = np.array([xmin, ymin, zmin], dtype=np.float64)
    bmax = np.array([xmax, ymax, zmax], dtype=np.float64)

    print("loading stl")
    triangles = load_stl(input_file)

    out_tris = []

    for idx, tri in enumerate(triangles):
        print(f"processing {idx} / {len(triangles)}")
        clipped_poly = clip_triangle(tri, bmin, bmax)
        if not clipped_poly:
            continue

        tris = triangulate(clipped_poly)
        for t in tris:
            if len(set(tuple(v) for v in t)) == 3:
                out_tris.append(np.array(t))

    out_tris = np.array(out_tris)

    print("saving stl")
    save_stl("clipped.stl", out_tris)

    print(f"Saved clipped.stl ({len(out_tris)} triangles)")

if __name__ == "__main__":
    main()
