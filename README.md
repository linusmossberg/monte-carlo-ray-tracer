# Monte Carlo Ray Tracer

This is a multithreaded ray tracer that achieves global illumination using the Monte Carlo based methods *Path Tracing* and *Photon Mapping*. This program was developed over a period of about 2 months for the course [Advanced Global Illumination and Rendering (TNCG15)](https://liu.se/studieinfo/kurs/tncg15) at Linköpings Universitet. The program is written in C++ and frequently makes use of C++17.

![](renders/c1_64sqrtspp_report_4k_flintglass_downscaled.png "Path Traced, Scene IOR 1.75")

## Report

A report describing this work in more detail is available [here](report.pdf). 

Due to length limitations, I mostly focused on my own solutions and things that sets my implementation apart from the course syllabus. The report therefore references but doesen't explicitly explain a lot of standard topics such as: ray-surface intersections (Möller Trumbore etc.), ray reflection and refractions (Snell's law etc.), Schlick's approximation of Fresnel factor, Oren-Nayar BRDF etc. Having an understanding of these topics are therefore prerequisites for the report.

## Scene Format

I created a scene file format for this project to simplify scene creation. The format is defined using JSON and I used the library [nlohmann::json](https://github.com/nlohmann/json) for JSON parsing. A complete scenefile example can be seen at [scenes/hexagon_room.json](scenes/hexagon_room.json).

The basic outline of the scene format is the following JSON object:

```json
{
  "num_render_threads": -1,
  "naive": false,
  "ior": 1.75,

  "photon_map": { },

  "cameras": [ ],

  "materials":  [ ],

  "vertices": [ ],

  "surfaces": [ ]
}
```

The **num_render_threads** field defines the number of rendering threads to use. This is limited between 1 and the number of concurrent threads availible on the system, i.e. the number of effective CPU cores. If the specified value is outside of this range, then all concurrent threads are used automatically.

The **naive** field specifies whether or not naive path tracing should be used rather than using explicit direct light sampling. There's no good reason to set this to true other than to test the massive difference in convergence time.

The **ior** field specifies the scene IOR (index of refraction). This can be used to simulate different types of environment mediums to see the effects this has on the angle of refraction and the Fresnel factor. This is usually set to 1.0 to simulate air, but using values such as 1.333 will render the scene as if it was submerged in water instead.

The **photon_map**, **cameras**, **materials**, **vertices**, and **surfaces** objects defines different render settings and the scene contents. I will go through each of these in the following sections.

### Photon Map Object

Example:
```json
"photon_map": {
  "emissions": 1e6,
  "caustic_factor": 100.0,
  "radius": 0.1,
  "caustic_radius": 0.05,
  "max_photons_per_octree_leaf": 190,
  "direct_visualization": false
}
```

The **photon_map** object is optional and it defines the photon map properties. The **emissions** field determines the base number of rays that should be emitted from light sources. More emissions will result in more spawned photons. 

The **caustic_factor** determines how many times more caustic photons should be generated relative to other photon types. 1 is the "natural" factor, but this results in blurry caustics since the caustic photon map is visualized directly.

The **radius** field determines the radius of the search sphere (in meters) used during the rendering pass. Smaller values results in sharper results and faster evaluation, but too small values results in bad estimates since this reduces the number of photons that contributes to the estimate. **caustic_radius** is the same but is used exclusively for caustic photons.

The **max_photons_per_octree_leaf** field affects both the octree radius-search performance and memory usage of the application. I cover this more in the report and this value can probably be left at 190 in most cases.

The **direct_visualization** field can be used to visualize the photon maps directly. Setting this to true will make the program evaluate the global radiance from all photon maps at the first diffuse reflection. An example of this is in the report. 

### Cameras Object

Example:
```json
"cameras": [
  {
    "focal_length": 23,
    "sensor_width": 35,
    "eye": [ -2, 0, 0 ],
    "look_at": [ 13, -0.55, 0 ],
    "width": 960,
    "height": 720,
    "sqrtspp": 4,
    "savename": "c1b"
  },
  {
    "focal_length": 50,
    "sensor_width": 35,
    "eye": [ -1, 0, 0 ],
    "forward": [ 1, 0, 0 ],
    "up": [ 0, 1, 0 ],
    "width": 960,
    "height": 540,
    "sqrtspp": 1,
    "savename": "c2"
  }
]
```

The **cameras** object contains an array of different cameras. The **focal_length** and **sensor_width** fields are defined in millimeters. A sensor width of 35mm (full frame) is most often usefull since focal lengths normally are defined in terms of 35mm-equivalent focal lengths.

The **eye** field defines the position of the camera, and the **up** and **forward** fields defines the orientation vectors of the camera. The up and forward vectors can be replaced with the **look_at** field, which defines the coordinate that the camera should look at instead.

The **width** and **height** properties are the dimensions of the sensor/image in terms of pixels. 

The **sqrtspp** (Square-Rooted Samples Per Pixel) property defines the square-rooted number of ray paths that should be sampled from each pixel in the camera.

The **savename** property defines the name of the resulting saved image file. Images are saved in Truevision TGA format with the *.tga* extension.

### Materials Object

Example:
```json
"materials": [
  {
    "name": "default",
    "reflectance": [ 0.5, 0.5, 0.5 ]
  },
  {
    "name": "red",
    "reflectance": [ 1.0, 0.218, 0.218 ],
    "roughness": 10.0
  },
  {
    "name": "crystal",
    "ior": 2.0,
    "transparency":  1.0,
    "specular_reflectance": [ 0.5, 1.0, 0.9 ]
  },
  {
    "name": "light",
    "reflectance": [ 0.8, 0.8, 0.8 ],
    "emittance": [ 1000, 1000, 1000 ]
  }
]
```

The **materials** object contains an array of different materials. The material field **name** can be any string and it's used later when assigning a material to a surface. The **"default"** name is used for the material that should be used on all surfaces that hasn't specified a material.

The remaining material fields are:

| field                | type       | default value |
| -------------------- | ---------- | ------------- |
| reflectance          | RGB vector | [0 0 0]       |
| specular_reflectance | RGB vector | [0 0 0]       |
| emittance            | RGB vector | [0 0 0]       |
| ior                  | scalar     | Scene IOR     |
| roughness            | scalar     | 0             |
| transparency         | scalar     | 0             |
| perfect_mirror       | boolean    | false         |

These fields are all optional and any combination of fields can be used. A material can for example be a combination of diffusely reflecting, specularly reflecting, emissive, transparent (specularly refracting) and rough. If the IOR is specified to be the same as the scene IOR, then the material is assumed to be completely diffuse. If set to true, the **perfect_mirror** field overrides most other fields to simulate a perfect mirror with infinite IOR.

The **reflectance** and **specular_reflectance** fields specifies the amount of radiance that should be diffusely and specularly reflected for each RGB channel. This is a simplification since radiance and reflectances are spectral properties that varies with wavelength and not by the resulting tristimulus values of the virtual camera, but this is computationally cheaper and simpler. The reflectance properties are linear and are defined in the range [0,1]. Color pickers usually display gamma corrected color values, which means that these has to be linearized before using them here to achieve the same color.

The **emittance** field defines the emittance of each RGB channel in flux. This means that surfaces of different sizes will emit the same amount of total radiance if they are assigned the same emissive material. 

### Vertices Object

Example:
```json
"vertices": [
  [
    [ 8, 4.9, -2.5 ],
    [ 9, 4.9, -2.5 ],
    [ 9, 4.9, -1.5 ],
    [ 8, 4.9, -1.5 ]
  ],
  [
    [ 8.28362, -5.0, -4.78046 ],
    [ 6.47867, -0.90516, -3.67389 ],
    [ 7.97071, -0.85108, -2.79588 ],
    [ 7.93553, -0.41379, -4.47145 ],
    [ 6.63966, 3.55331, -2.51368 ]
  ]
]
```

The **vertices** object contains an array of vertex sets. Each vertex set contains an array of vertices specified as xyz-coordinates. The vertex sets are implicitly assigned an index starting from 0 depending on their order in the array. In the above example the first set has the index 0 and the second the index 1.

The vertex set index is used later to specify which set of vertices to build the surface from when creating surfaces of **object** types. The first set in the above example defines 4 vertices that makes up the square light source seen in the hexagon room renders, while the second set defines the 5 vertices that makes up the crystal object.

### Surfaces Object

Example:
```json
"surfaces": [
  {
    "type": "object",
    "material": "light",
    "set": 0,
    "triangles": [
      [ 0, 1, 2 ],
      [ 0, 2, 3 ]
    ]
  },
  {
    "type": "object",
    "material": "crystal",
    "set": 1,
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
  }
]
```

The **surfaces** object contains an array of surfaces. Each surface has a **type** field which can be either **"sphere"**, **"triangle"** or **"object"**. All surfaces also has an optional **material** field, which specifies the material that the surface should use by name. 

#### Type-specific fields:

**Sphere:**<br>
The sphere position is defined by the **origin** field, while the sphere radius is defined by the **radius** field.

**Triangle:**<br>
The triangle is simply defined by its vertices, which is defined by the 3 vertices in the vertex array **vertices** in xyz-coordinates. The order of the vertices defines the normal direction, but this only matters if the surface has an emissive material.

**Object:**<br>
The **object** surface type defines a triangle mesh object that consists of multiple triangles. The **set** field defines the index of the vertex set to pull vertices from, while the **triangles** field specifies the array of triangles of the object. Each triangle of the array consists of 3 indices that references the corresponding vertex index in the vertex set.

## Usage Walkthrough

The program should be started in the directory that contains the *scenes* directory. Most settings are defined in the scene files.

When you first start the program, you're greeted with a menu similar to this:
```
Scene directory:
C:\Users\Laptop\Documents\TNCG15\monte-carlo-ray-tracer\scenes

 ____________________________________________________________________________________
| Scene option | File                 | Camera                                       |
|______________|______________________|______________________________________________|
| 0            | hexagon_room         | Eye: (-2 0 0), Focal length: 23mm (35mm)     |
|______________|______________________|______________________________________________|
| 1            | hexagon_room         | Eye: (-1 0 0), Focal length: 24mm (35mm)     |
|______________|______________________|______________________________________________|
| 2            | hexagon_room         | Eye: (-3 1 0), Focal length: 19mm (35mm)     |
|______________|______________________|______________________________________________|
| 3            | hexagon_room         | Eye: (-1 0 0), Focal length: 120mm (35mm)    |
|______________|______________________|______________________________________________|
| 4            | hexagon_room_diffuse | Eye: (-2 0 0), Focal length: 21mm (35mm)     |
|______________|______________________|______________________________________________|
| 5            | oren_nayar_test      | Eye: (-80 40 80), Focal length: 300mm (35mm) |
|______________|______________________|______________________________________________|

Select scene option:
```

One scene option is created for each camera in the scene files. Select the scene you want to render by entering the scene option number and pressing enter.
```
Select scene option: 0

Threads used for rendering: 4

Use photon mapping? (y/n)
```

If the selected scene option has a photon map defined you'll get this query. Enter either *y* if you want to use photon mapping or *n* otherwise and press enter. At this point the program will start creating the photon map and the progress will be printed to the console. If no photon map is present or if *n* is selected, the program will jump directly to the main rendering pass. No more input is required after this point.
```
Use photon mapping? (y/n) y

----------------------------| PHOTON MAPPING PASS |----------------------------

Total number of photon emissions from light sources: 25 000 000

Photons emitted: 10.80%
```

Once the photon maps have been constructed, a bunch of stats is printed to the console and then the main rendering pass starts. 
```
----------------------------| PHOTON MAPPING PASS |----------------------------

Total number of photon emissions from light sources: 25 000 000

Photons emitted in 00:01:07. Octrees constructed in 00:00:04.

Photon maps and numbers of stored photons:

   Direct photons: 918 163
 Indirect photons: 1 150 005
  Caustic photons: 2 908 859
   Shadow photons: 677 229

----------------------------| MAIN RENDERING PASS |----------------------------

Samples per pixel: 16
```

The main rendering pass prints more verbose progress information. Time remaining is the estimated hours, minutes and seconds remaining until the render is completed. ETA is the estimated completion date and time. Finally, Samples/s is the number of complete ray paths that the program is currently calculating per second.
```
Time remaining: 00:06:16 || 18.00% || ETA: 2020-02-11 11:08 || Samples/s: 424 057
```

Once the render has completed, stats about the elapsed time is printed to the console. The program then waits for the user to press enter before exiting the program. This is done to prevent the console from closing and erasing all stats before the user has seen them.
```
Render Completed: 2020-02-11 11:09, Elapsed Time: 00:08:43

Press enter to exit.
```

Note: The program makes heavy use of the *\\r* carriage return character for console output in order to print progress information on the same line. This may work differently on different platforms which may mess up the output.

## Renders

The following images are renders of [scenes/hexagon_room.json](scenes/hexagon_room.json) and [scenes/oren_nayar_test.json](scenes/oren_nayar_test.json) produced by the program:

___

<h3 align="center">Path Traced, Scene IOR 1.0</h3>

![](renders/c1_64sqrtspp_report_4k_downscaled.png "Path Traced, Scene IOR 1")

___

<h3 align="center">Path Traced, Scene IOR 1.75</h3>

![](renders/c1_64sqrtspp_report_4k_flintglass_downscaled.png "Path Traced, Scene IOR 1.75")

___

<h3 align="center">Photon Mapped, Scene IOR 1.0</h3>

![](renders/c1_photon-map_report_2e6_250_16sqrtspp.png "Photon Mapped")

___

<h3 align="center">Path Traced, Skylit Oren-Nayar Spheres</h3>

![](renders/oren_nayar_test.png "Path Traced, Skylit Oren-Nayar Spheres")

## Compiling

Currently only Visual Studio 2019 project files are included. Compiling on other platforms should be straightforward however, just compile the *.cpp* files in [source](source) with [external/glm](external/glm) and [external/nlohmann](external/nlohmann) directories added to the include path. Requires a compiler with C++17 support.
