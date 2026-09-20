"""glTF 2.0 source decoding and affine math, used only by the offline cooker."""
import base64
import json
import math
from pathlib import Path
import struct
from urllib.parse import unquote

IDENTITY = [1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.]


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def unit(v):
    length = math.sqrt(dot(v, v))
    if not math.isfinite(length) or length < 1e-12:
        raise ValueError('zero or non-finite direction/quaternion')
    return [x / length for x in v]


def cross(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]


def mul(a, b):
    return [sum(a[r * 4 + k] * b[k * 4 + c] for k in range(4)) for r in range(4) for c in range(4)]


def inverse(matrix):
    rows = [[*matrix[r * 4:r * 4 + 4], *IDENTITY[r * 4:r * 4 + 4]] for r in range(4)]
    for column in range(4):
        pivot = max(range(column, 4), key=lambda r: abs(rows[r][column]))
        if abs(rows[pivot][column]) < 1e-12:
            raise ValueError('singular asset transform')
        rows[column], rows[pivot] = rows[pivot], rows[column]
        scale = rows[column][column]
        rows[column] = [v / scale for v in rows[column]]
        for r in range(4):
            if r != column:
                scale = rows[r][column]
                rows[r] = [v - scale * p for v, p in zip(rows[r], rows[column])]
    return [v for row in rows for v in row[4:]]


def point(matrix, value, w=1):
    return [sum(matrix[r * 4 + c] * value[c] for c in range(3)) + matrix[r * 4 + 3] * w for r in range(3)]


def normal(matrix, value):
    inv = inverse(matrix)
    return unit([sum(inv[c * 4 + r] * value[c] for c in range(3)) for r in range(3)])


def trs(translation, rotation, scale):
    x, y, z, w = unit(rotation)
    result = [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w), translation[0],
              2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w), translation[1],
              2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y), translation[2],
              0, 0, 0, 1]
    for r in range(3):
        for c in range(3):
            result[r * 4 + c] *= scale[c]
    return result


def decompose(m):
    columns = [[m[r * 4 + c] for r in range(3)] for c in range(3)]
    scale = [math.sqrt(dot(v, v)) for v in columns]
    if min(scale) < 1e-12:
        raise ValueError('zero-scale skeletal transform')
    if dot(cross(columns[0], columns[1]), columns[2]) < 0:
        scale[0] = -scale[0]
    rotation = [[columns[c][r] / scale[c] for c in range(3)] for r in range(3)]
    if any(abs(dot([rotation[r][a] for r in range(3)], [rotation[r][b] for r in range(3)])) > 1e-4
           for a, b in ((0, 1), (0, 2), (1, 2))):
        raise ValueError('skeletal shear cannot be represented by IQM TRS joints')
    trace = sum(rotation[i][i] for i in range(3))
    if trace > 0:
        s = math.sqrt(trace + 1) * 2
        q = [(rotation[2][1] - rotation[1][2]) / s, (rotation[0][2] - rotation[2][0]) / s,
             (rotation[1][0] - rotation[0][1]) / s, s / 4]
    else:
        i = max(range(3), key=lambda n: rotation[n][n])
        j, k = (i + 1) % 3, (i + 2) % 3
        s = math.sqrt(max(0, 1 + rotation[i][i] - rotation[j][j] - rotation[k][k])) * 2
        q = [0., 0., 0., (rotation[k][j] - rotation[j][k]) / s]
        q[i], q[j], q[k] = s / 4, (rotation[j][i] + rotation[i][j]) / s, (rotation[k][i] + rotation[i][k]) / s
    return [m[3], m[7], m[11]], unit(q), scale


def slerp(a, b, t):
    a, b = unit(a), unit(b)
    cosine = dot(a, b)
    if cosine < 0:
        b, cosine = [-v for v in b], -cosine
    if cosine > 0.9995:
        return unit([x + t * (y - x) for x, y in zip(a, b)])
    angle = math.acos(max(-1, min(1, cosine)))
    return [(x * math.sin((1 - t) * angle) + y * math.sin(t * angle)) / math.sin(angle) for x, y in zip(a, b)]


class Document:
    def __init__(self, path, read):
        self.path, self.read = path, read
        data = read(path)
        binary = None
        if data[:4] == b'glTF':
            magic, version, length = struct.unpack_from('<III', data)
            if version != 2 or length != len(data):
                raise ValueError('GLB version/length is not supported')
            offset, chunks = 12, {}
            while offset < len(data):
                size, kind = struct.unpack_from('<II', data, offset)
                offset += 8
                if size % 4 or size > len(data) - offset or kind in chunks:
                    raise ValueError('invalid GLB chunk layout')
                chunks[kind] = data[offset:offset + size]
                offset += size
            data, binary = chunks[0x4E4F534A], chunks.get(0x004E4942)
        self.data = json.loads(data)
        if self.data.get('asset', {}).get('version') != '2.0':
            raise ValueError('expected a glTF 2.0 asset')
        if self.data.get('extensionsRequired'):
            raise ValueError('required glTF extensions are not supported: ' + ', '.join(self.data['extensionsRequired']))
        self.buffers = []
        for index, buffer in enumerate(self.data.get('buffers', [])):
            value = self.uri(buffer['uri']) if 'uri' in buffer else binary if index == 0 else None
            if value is None or len(value) < buffer['byteLength']:
                raise ValueError('missing or short glTF buffer')
            self.buffers.append(value[:buffer['byteLength']])
        self.cache = {}

    def uri(self, uri):
        if uri.startswith('data:'):
            header, data = uri.split(',', 1)
            if not header.endswith(';base64'):
                raise ValueError('only base64 embedded glTF data is supported')
            return base64.b64decode(data, validate=True)
        if ':' in uri or '?' in uri or '#' in uri:
            raise ValueError('glTF external sources must be local relative paths')
        return self.read(self.path.parent / unquote(uri))

    def view(self, index):
        view = self.data['bufferViews'][index]
        buffer = self.buffers[view['buffer']]
        offset, size = view.get('byteOffset', 0), view['byteLength']
        if offset < 0 or size < 0 or size > len(buffer) - offset:
            raise ValueError('glTF buffer view exceeds its buffer')
        return buffer[offset:offset + size]

    def values(self, view_index, offset, count, component, shape, stride=None):
        fmt, size = {5120: ('b', 1), 5121: ('B', 1), 5122: ('h', 2),
                     5123: ('H', 2), 5125: ('I', 4), 5126: ('f', 4)}[component]
        rows, columns = {'SCALAR': (1, 1), 'VEC2': (2, 1), 'VEC3': (3, 1), 'VEC4': (4, 1),
                         'MAT2': (2, 2), 'MAT3': (3, 3), 'MAT4': (4, 4)}[shape]
        column_bytes = (rows * size + 3) // 4 * 4 if columns > 1 else rows * size
        element_bytes = column_bytes * columns
        stride = stride or element_bytes
        view = self.view(view_index)
        if count < 0 or offset < 0 or stride < element_bytes or offset + max(0, count - 1) * stride + (element_bytes if count else 0) > len(view):
            raise ValueError('glTF accessor exceeds its buffer view')
        return [[struct.unpack_from('<' + fmt, view, offset + i * stride + c * column_bytes + r * size)[0]
                 for c in range(columns) for r in range(rows)] for i in range(count)]

    def accessor(self, index):
        if index in self.cache:
            return self.cache[index]
        a = self.data['accessors'][index]
        count, component, shape = a['count'], a['componentType'], a['type']
        components = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT2': 4, 'MAT3': 9, 'MAT4': 16}[shape]
        if not 0 <= count <= 16000000:
            raise ValueError('glTF accessor exceeds the cooker element limit')
        if 'bufferView' in a:
            view = self.data['bufferViews'][a['bufferView']]
            result = self.values(a['bufferView'], a.get('byteOffset', 0), count, component, shape, view.get('byteStride'))
        else:
            result = [[0] * components for _ in range(count)]
        if 'sparse' in a:
            sparse = a['sparse']
            indices, values = sparse['indices'], sparse['values']
            keys = self.values(indices['bufferView'], indices.get('byteOffset', 0), sparse['count'], indices['componentType'], 'SCALAR')
            replacements = self.values(values['bufferView'], values.get('byteOffset', 0), sparse['count'], component, shape)
            previous = -1
            for (key,), value in zip(keys, replacements):
                if not previous < key < count:
                    raise ValueError('sparse glTF indices must be ordered and in range')
                result[key], previous = value, key
        if a.get('normalized') and component != 5126:
            maximum = {5120: 127, 5121: 255, 5122: 32767, 5123: 65535}[component]
            result = [[max(-1, value / maximum) for value in row] for row in result]
        if any(not math.isfinite(value) for row in result for value in row):
            raise ValueError('non-finite glTF accessor value')
        self.cache[index] = result
        return result

    def image(self, index):
        image = self.data['images'][index]
        return self.uri(image['uri']) if 'uri' in image else self.view(image['bufferView'])
