# Monte Carlo Ray Tracer

This is a physically based renderer with Path Tracing and Photon Mapping.

<div about="renders/stanford_dragon.jpg">
  <img src="renders/stanford_dragon.jpg" alt="Path traced render of the stanford dragon, 871 414 triangles." title="Path traced render of the stanford dragon, 871 414 triangles." />
  <a rel="license" href="https://creativecommons.org/licenses/by/4.0/"></a>
</div>
<div about="renders/quadric.jpg">
  <img src="renders/quadric.jpg" alt="Path traced render of scene with only quadric surfaces" title="Path traced render of scene with only quadric surfaces" />
  <a rel="license" href="https://creativecommons.org/licenses/by/4.0/"></a>
</div>
<div about="renders/caustics.jpg">
  <img src="renders/caustics.jpg" alt="Photon mapped render of caustics, 6 583 508 triangles. Original scene by Benedikt Bitterli." title="Photon mapped render of caustics, 6 583 508 triangles. Original scene by Benedikt Bitterli." />
  <a rel="license" href="https://creativecommons.org/licenses/by/4.0/"></a>
</div>

This program was initially developed over a period of about 2 months for the course [Advanced Global Illumination and Rendering (TNCG15)](https://liu.se/studieinfo/kurs/tncg15) at Linköpings Universitet. I have however continued developing it by adding features such as depth of field and quadric surfaces. The program is written in C++ and requires a compiler with C++17 support.

## Usage

For basic use, just run the program in the directory that contains the *scenes* directory, i.e. the root folder of this repository. The program will then parse all scene files and create several rendering options to choose from in the terminal. For more advanced use, see [scene format](#scene-format).

## Scene Format

I created a scene file format for this project to simplify scene creation. The format is defined using JSON and I used the library [nlohmann::json](https://github.com/nlohmann/json) for JSON parsing. Complete scene file examples can be found in the scenes directory.

The basic outline of the scene format is the following JSON object:

```json
{
  "num_render_threads": -1,
  "naive": false,
  "ior": 1.75,

  "photon_map": { },
  "bvh": { },
  "cameras": [ ],
  "materials":  { },
  "vertices": { },
  "surfaces": [ ]
}
```

The `num_render_threads` field specifies the number of rendering threads to use. This is limited between 1 and the number of concurrent threads available on the system. All concurrent threads are used if the specified value is outside of this range.

The `naive` field specifies whether naive path tracing should be used rather than using explicit direct light sampling.

The `ior` field specifies the scene IOR (index of refraction). This can be used to simulate different types of environment mediums to see the effects this has on the angle of refraction and the Fresnel factor.

The `photon_map`, `bvh`, `cameras`, `materials`, `vertices`, and `surfaces` objects specifies different render settings and scene contents. I go through each of these in the following sections. Click the summaries for more details.

### Photon Map

<details><summary>The <code>photon_map</code> object is optional and it specifies the photon map properties.</summary><br>

Example:
```json
"photon_map": {
  "emissions": 1e6,
  "caustic_factor": 100.0,
  "k_nearest_photons": 50,
  "max_radius": 0.01,
  "max_caustic_radius": 0.005,
  "max_photons_per_octree_leaf": 190,
  "use_shadow_photons": false,
  "direct_visualization": false
}
```

The `emissions` field determines the base number of rays that should be emitted from light sources. More emissions will result in more spawned photons. 

The `caustic_factor` determines how many times more caustic photons should be generated relative to other photon types. 1 is the "natural" factor, but this results in blurry caustics since the caustic photon map is visualized directly.

The `k_nearest_photons` field specifies the number of nearest photons to search for and use in the radiance estimate each time a photon map is evaluated at a point. Larger values create better but less localized (blurrier) estimates since the search sphere is expanded to cover the target number of photons. The maximum radius of this search sphere is controlled with the `max_radius` field. This is useful to discard large parts of the search space and thereby increase performance. `max_caustic_radius` is the same but is used exclusively for caustic photons.

The `max_photons_per_octree_leaf` field affects both the octree radius-search performance and memory usage of the application. I cover this more in the report and this value can probably be left at 190 in most cases.

The `use_shadow_photons` specifies whether to use shadow photons. In the current implementation, shadow photons are only used to determine if the global radiance evaluation should be delayed on secondary diffuse reflections. This used to help in reducing artifacts, but it doesn't seem to help much with the photon map improvements I've made since then and it can increase render times significantly in some scenes. 

The `direct_visualization` field can be used to visualize the photon maps directly. Setting this to true will make the program evaluate the global radiance from all photon maps at the first diffuse reflection. An example of this is in the report. 
</details>

___

### BVH

<details><summary>The <code>bvh</code> object is optional and it specifies the Bounding Volume Hierarchy acceleration structure properties.</summary><br>

Example:
```json
"bvh": {
    "type": "quaternary_sah",
    "bins_per_axis": 16
}
```

Normal naive scene intersection is used if this object is not specified. The `type` field specifies the hierarchy method to use when constructing the tree.

| `type`  | Method | 
| ------- | ------ | 
| `octree` | First creates an octree by iterative insertion of the primitive centroids, and then transforms this tree into a BVH by just transferring the octree node hierarchy and computing the bounding boxes. | 
| `binary_sah` | Creates a binary-tree BVH by recursively splitting the primitives into two groups. The split occurs along the axis with the largest primitive centroid extent, and the split position is determined by the Surface Area Heuristic (SAH). Binning is performed to reduce the number of evaluated split coordinates along the axis, and the number of bins is determined by the `bins_per_axis` field. | 
| `quaternary_sah` | Creates a quaternary-tree BVH by recursively splitting the primitives into the four groups that results in the lowest SAH-cost. This is similar to the binary version, but the split now occurs along two axes. The bins form a regular 2D grid and `bins_per_axis`<sup>2</sup> possible split coordinates are evaluated. |

I've also tried splitting along all three axes each recursion to create octonary-trees. This produces good results but there's not much of an improvement compared to the quaternary version and the construction time becomes much longer due to the dimensionality curse when using 3D bins.

`quaternary_sah` takes the longest to construct but tends to produce the best results. `octree` and `binary_sah` are faster to construct which is useful for quick renders. This is especially the case for the octree method, which surprisingly seems to be both faster to construct and create higher quality trees than the binary-tree SAH method.

</details>

___

### Cameras

<details><summary>The <code>cameras</code> object contains an array of different cameras</summary><br>

Example:
```json
"cameras": [
  {
    "focal_length": 23,
    "sensor_width": 35,
    "f_stop": 1.8,
    "eye": [ -2, 0, 0 ],
    "look_at": [ 13, -0.55, 0 ],
    "image": { 
      "width": 960, 
      "height": 720, 
      "exposure_compensation": -1, 
      "gain_compensation": 0.5 
    },
    "sqrtspp": 4,
    "savename": "c1b"
  },
  {
    "focal_length": 50,
    "sensor_width": 35,
    "f_stop": 5.6,
    "focus_distance": 3,
    "eye": [ -1, 0, 0 ],
    "forward": [ 1, 0, 0 ],
    "up": [ 0, 1, 0 ],
    "image": { 
      "width": 960, 
      "height": 540 
    },
    "sqrtspp": 1,
    "savename": "c2"
  }
]
```

The `focal_length` and `sensor_width` fields are defined in millimeters. A sensor width of 35mm (full frame) is most often useful since focal lengths normally are defined in terms of 35mm-equivalent focal lengths.

The `eye` field defines the position of the camera, and the `up` and `forward` fields defines the orientation vectors of the camera. The up and forward vectors can be replaced with the `look_at` field, which defines the coordinate that the camera should look at instead.

The `f_stop` and `focus_distance` fields defines the depth of field properties of the camera and are optional. The distance from the camera to the `look_at` coordinate is used as focus distance if this coordinate is specified and if no valid focus distance is specified.

The `sqrtspp` (Square-Rooted Samples Per Pixel) property defines the square-rooted number of ray paths that should be sampled from each pixel in the camera.

The `savename` property defines the name of the resulting saved image file. Images are saved in TGA format.

The `image` object specifies the image properties of the camera. The `width` and `height` ´fields specifies the image resolution in pixels.

The program has histogram-based auto-exposure which centers the histogram around the 0.5 intensity level before applying tone mapping (i.e. it corresponds to controlling the amount of light that reaches the film/sensor). This can be offset with the optional `exposure_compensation` field of the `image` object, which specifies the [exposure compensation](https://en.wikipedia.org/wiki/Exposure_compensation) in EV units (stops). 

The program also has a histogram-based auto-gain method which is applied after auto-exposure and tone-mapping, which instead tries to position the histogram of the resulting image to the right. This can similarly be offset with the optional `gain_compensation` field, which is also specified in EV units.

The reason for separating these steps is that the tone-mapping/camera response is non-linear, and as a result `exposure_compensation` mostly controls the camera response (contrast, dynamic range etc.) while `gain_compensation` controls the overall image intensity. The tone mapping operator used by the program is the [filmic tonemapper](http://filmicworlds.com/blog/filmic-tonemapping-operators/) developed by John Hable.
</details>

___

### Materials

<details><summary>The <code>materials</code> object contains a map of different materials.</summary><br>

Example:
```json
"materials": {
  "default": {
      "reflectance": [ 0.73, 0.73, 0.73 ]
  },
  "red": {
    "reflectance": "#FF8080",
    "roughness": 10.0
  },
  "crystal": {
    "ior": 2.0,
    "transparency":  1.0,
    "specular_reflectance": [ 0.5, 1.0, 0.9 ]
  },
  "one_sheet_hyperboloid": {
    "specular_reflectance": "#FFFFFF",
    "ior": 1.333,
    "reflectance": "#80B1D3"
  },
  "light": {
    "reflectance": [ 0.9, 0.9, 0.9 ],
    "emittance": [ 1000, 1000, 1000 ]
  }
}
```

The key string is used later when assigning a material to a surface. The material with the `default` key string is used for all surfaces that hasn't specified a material.

The material fields are:

| field                | type        | default value |
| -------------------- | ----------- | --------------|
| reflectance          | RGB vector  | [0 0 0]       |
| specular_reflectance | RGB vector  | [0 0 0]       |
| emittance            | RGB vector  | [0 0 0]       |
| ior                  | scalar      | -1            |
| roughness            | scalar      | 0             |
| transparency         | scalar      | 0             |
| perfect_mirror       | boolean     | false         |

These fields are all optional and any combination of fields can be used. A material can for example be a combination of diffusely reflecting, specularly reflecting, emissive, transparent (specularly refracting) and rough. If the IOR is specified to be the less than 1, then the material is assumed to be completely diffuse. If set to true, the `perfect_mirror` field overrides most other fields to simulate a perfect mirror with infinite IOR.

The `reflectance` and `specular_reflectance` fields specifies the amount of radiance that should be diffusely and specularly reflected for each RGB channel. This is a simplification since radiance and reflectances are spectral properties that varies with wavelength and not by the resulting tristimulus values of the virtual camera, but this is computationally cheaper and simpler. The reflectance properties are defined in the range `[0,0,0]` to `[1,1,1]`, or `#000000` to `#FFFFFF` if a hex string is used. The reflectance properties now take gamma-corrected values and linearizes them internally to make it easier to pick colors via color pickers (which usually display gamma corrected values).

The `emittance` field defines the radiant flux of each RGB channel in watts. This means that surfaces with different surface areas will emit the same amount of radiant energy if they are assigned the same emissive material.
</details>

___

### Vertices

<details><summary>The <code>vertices</code> object contains a map of vertex sets.</summary><br>

Example:
```json
"vertices": {
  "light": [
    [ 8, 4.9, -2.5 ],
    [ 9, 4.9, -2.5 ],
    [ 9, 4.9, -1.5 ],
    [ 8, 4.9, -1.5 ]
  ],
  "crystal": [
    [ 8.28362, -5.0, -4.78046 ],
    [ 6.47867, -0.90516, -3.67389 ],
    [ 7.97071, -0.85108, -2.79588 ],
    [ 7.93553, -0.41379, -4.47145 ],
    [ 6.63966, 3.55331, -2.51368 ]
  ]
}
```

Each vertex set contains an array of vertices specified as xyz-coordinates. The vertex set key string is used later to specify which set of vertices to build the surface from when creating surfaces of `object` type.
</details>

___

### Surfaces

<details><summary>The <code>surfaces</code> object contains an array of surfaces.</summary><br>

Example:
```json
"surfaces": [
  {
      "type": "object",
      "file": "data/stanford_dragon.obj"
  },
  {
    "type": "object",
    "material": "light",
    "vertex_set": "light",
    "triangles": [
      [ 0, 1, 2 ],
      [ 0, 2, 3 ]
    ]
  },
  {
    "type": "object",
    "material": "crystal",
    "vertex_set": "crystal",
    "triangles": [
      [ 0, 2, 1 ],
      [ 0, 3, 2 ],
      [ 0, 1, 3 ],
      [ 2, 4, 1 ],
      [ 1, 4, 3 ],
      [ 3, 4, 2 ]
    ]
  },
  {
    "type": "sphere",
    "origin": [ 9.25261, -3.70517, -0.58328 ],
    "radius": 1.15485
  },
  {
    "type": "triangle",
    "material":  "red",
    "vertices": [ 
      [ 9, 4.9, -2.5 ],
      [ 9, 4.9, -1.5 ],
      [ 8, 4.9, -1.5 ]
    ]
  },
  {
    "type": "quadric",
    "material": "one_sheet_hyperboloid",
    "XX": -1, "YY": 1, "ZZ": 1, "R": -1,
    "bound_dimensions": [1.0, 0.2, 0.2],
    "origin": [0.3, 0.3, 0.125],
    "scale": 0.025,
    "orientation": { "axis": [1,0,0], "angle": 45 }
  }
]
```

Each surface has a `type` field which can be either `sphere`, `triangle`, `object` or `quadric`. All surfaces also has an optional `material` field, which specifies the material that the surface should use by material key string. The remaining fields are type specific.

#### Sphere
The sphere position is defined by the `origin` field, while the sphere radius is defined by the `radius` field.

#### Triangle
The triangle is simply defined by its vertices, which is defined by the 3 vertices in the vertex array `vertices` in xyz-coordinates. The order of the vertices defines the normal direction, but this only matters if the surface has an emissive material.

#### Object
The object surface type defines a triangle mesh object that consists of multiple triangles. The `vertex_set` field can be used to specify the key string of the vertex set to pull vertices from, and the `triangles` field then specifies the array of triangles of the object. Each triangle of the array consists of 3 indices that references the corresponding vertex index in the vertex set. Alternatively, the `file` field can be used to specify a path to an OBJ-file to load instead. The path should be relative to the scenes directory. The program uses normal interpolation for smooth shading if the OBJ file specifies vertex normals for the mesh.

#### Quadric
A quadric surface consists of all points (x,y,z) that satisfies the quadric equation<sup>1</sup>:

<p align="center"><img  src="renders/quadric_eq.svg" height="20" /></p>

where A, B, C etc. are real constants. A sphere with radius 1 can for example be defined by:

<p align="center"><img  src="renders/sphere_eq.svg" height="20" /></p>

with constants J=-1, A=E=H=1 and the rest 0. This is achieved in the program by specifying the following fields for a quadric surface:
```json
"XX": 1, "YY": 1, "ZZ": 1, "R": -1,
```
Instead of the usual constant names, I've opted for more descriptive field names that correspond to the expression that the field value is multiplied with in the quadric equation. The `R` field corresponds to J in the quadric equation, i.e. the scalar constant added at the end. The value of unspecified constants are set to 0.

The `bound_dimensions` field specifies the dimensions of the axis-aligned bounding box that the quadric surface is confined to. This can be replaced by the `bound_size` field which specifies a single width used for all dimensions.

The `origin`, `scale` and `orientation` fields are optional and they are used to transform the quadric surface. The `axis` field of the `orientation` object specifies the vector that the surface should be rotated about `angle` degrees (curl right hand rule).

Quadric surfaces currently do not support emissive materials (the emissive part is simply ignored).

___
<sup>1</sup> The usual quadric equation looks slightly different when it's derived from the quadric matrix representation *p<sup>T</sup>Qp* since this results in some constants being doubled. The program uses this representation internally, but I've eliminated this in the scene format since it's easier to not have to think about whether or not some constants will be doubled when creating a surface.
</details>

___