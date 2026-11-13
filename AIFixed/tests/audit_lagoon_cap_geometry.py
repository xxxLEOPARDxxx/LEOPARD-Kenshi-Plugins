"""Reproduce the current Drone's rounded-cap footing from actual collision XML.

This geometry corroborates the logged normal, not nav connectivity or a safe exit.
"""
from pathlib import Path
import hashlib, json, math, xml.etree.ElementTree as ET
HERE = Path(__file__).resolve().parent
fixture = json.loads((HERE/'fixtures/lagoon_cap_20261113.json').read_text(encoding='utf-8'))
source = Path('D:/SteamLibrary/steamapps/common/Kenshi')/fixture['collision_source']
raw = source.read_bytes()
assert hashlib.sha256(raw).hexdigest() == fixture['collision_sha256']
root = ET.fromstring(raw)
shape = next(s for s in root.iter('NxCapsuleShapeDesc')
             if s.find('NxShapeDesc').get('name') == fixture['collision_shape'])
pose = list(map(float, shape.find('NxShapeDesc/localPose').text.split()))
radius, height = float(shape.get('radius')), float(shape.get('height'))

def cross(a,b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def rotate(v):
    # PhysX asset Z-up -> game's Y-up, then the live building quaternion.
    v = (v[0], v[2], -v[1])
    wxyz = fixture['support_orientation_wxyz']
    q = (*wxyz[1:], wxyz[0])  # Ogre stores quaternion scalar first.
    t = tuple(2*x for x in cross(q[:3],v))
    c = cross(q[:3], t)
    return tuple(v[i]+q[3]*t[i]+c[i] for i in range(3))

center_offset = rotate(pose[9:12])
center = tuple(a+b for a,b in zip(fixture['support_position'],center_offset))
axis = rotate(pose[1:8:3])  # PhysX capsule axis is matrix column Y.
endpoint = tuple(center[i]-axis[i]*height*.5 for i in range(3))
assert max(abs(a-b) for a,b in zip(endpoint,fixture['drone']['expected_capsule_endpoint'])) < 1e-6
point = fixture['drone']['position']
expanded_radius = radius+fixture['sphere_sweep_radius_world_units']
dx,dz = point[0]-endpoint[0],point[2]-endpoint[2]
dy = math.sqrt(expanded_radius**2-dx**2-dz**2)
normal_y = dy/expanded_radius
assert abs(normal_y-fixture['drone']['expected_analytic_normal_y']) < 1e-9
assert abs(normal_y-fixture['drone']['observed_footing_normal_y']) < .0005
assert .60 < normal_y < .95
print(f'AIFixed Lagoon cap geometry: PASS; actual COL013 endpoint normalY={normal_y:.9f} matches logged .846; native island/landing still require live validation')
