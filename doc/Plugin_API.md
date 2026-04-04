# Prusa Slicer Plugin API

This is WIP draft + description of sort of current implementation.

## Plugin anatomy

- Each plugin is single .lua file located under specific directory (e.g. `(datadir)/lua` or `(configdir)/lua`).
- Plugin file has to define `info` variable with description of the plugin.
- Plugin file has to define `execute` function, that runs the plugin logic.

### Plugin Metadata `info` structure

The table `info` describes plugin with following keys:
- `id` (string) plugin unique identifier, recommended is reverse domain name like notation
- `type` (string) type of plugin, at the moment only `'project.plugin'` is allowed.
- `title` (string) displayed plugin name
- `menu` (string) menu item path to register the plugin under _Plugins_ menu item (e.g. `Calibration/My cool pattern`)
- `params` (array) list of parameter descriptions with following keys:
  - `name` (string) name of key in table as first argument passed to the `execute()` function.
  - `label` (string) displayed name in UI 
  - `type` (string) type of value / UI control, allowed values are:
    - `float` (UI: number input)
    - `int` (UI: number input)
    - `bool` (UI: checkbox)
  - `default` (number or string) default value

### Plugin `execute` function

The function `execute(params)` takes single argument,a table based upon description in the `info.params`. 
Values was filled prior calling the function, by user in UI constructed according the same description.


### Complete minimal example

This is the legendary hello world as a Slicer Plugin:

```lua
info = {
    id = "com.prusa3d.slicer.hello_world",
    type = "project.plugin",
    title = "Hello world",
    menu = "Minimal/Hello world",
    params = {
        {name = "num", label = "Your lucky number", type = "int", default = 42}
    }
}

function execute(params) 
    print("Hello no " .. params.num .. "!")
end
```

Once scanned, it should appear in the main menu under _Plugins_ → _Minimal_ → _Hello world_. 
After activation a simple UI will appear, with single input item of given label, so the user can pass an integer number.
The number can be than read by script as `params.num` in the `print` statement.

### Security model

The plugin runtime is *intentionally limited and sandboxed*. 

There are two main restrictions to be aware of:
- no standard `os` and `io` modules are available,
- plugin can access (via `emboss_svg`, `load_stl` and `require`) only files that are in the same directory 
  as the plugin .lua file itself. 

## Plugin API

All Slicer API is located in `api` module

### Type `project.plugin`

#### Functions


##### `api.make_cube`

Create cube mesh with given dimensions

```lua
local width = 10 -- [mm]
local height = 10
local depth = 100

local mesh = api.make_cube(width, height, depth)
```

##### `api.make_sphere`

Create sphere mesh with given parameters

```lua
local radius = 30              -- [mm]
local angular_granularity = 15 -- [deg] optional, default = 1

local mesh = make_sphere(radius, angular_granularity)
```


##### `api.make_cylinder`

Create cylinder mesh with given parameters

```lua
local radius = 30              -- [mm]
local height = 50              -- [mm]
local angular_granularity = 15 -- [deg] optional, default = 1

local mesh = make_cylinder(radius, height, angular_granularity)
```


##### `api.make_cone`

Create cone mesh with given parameters

```lua
local radius = 30              -- [mm]
local height = 50              -- [mm]
local angular_granularity = 15 -- [deg] optional, default = 1

local mesh = make_cone(radius, height, angular_granularity)
```


##### `api.make_tetrahedron`

Create tetrahedron mesh with given parameters

```lua
local size = 10 -- [mm]

local mesh = make_tetrahedron(size)
```


##### `api.make_prism`

Create prism mesh with given parameters

```lua
local width  = 10 -- [mm]
local length = 20
local height = 30

local mesh = make_prism(width, length, height)
```


##### `api.make_frustum`

Create frustum mesh with given parameters

```lua
local radius = 30              -- [mm]
local height = 50              -- [mm]
local angular_granularity = 15 -- [deg] optional, default = 1

local mesh = make_frustum(radius, height, fa)
```


##### `api.make_frustum_dowel`

Create frustum_dowel mesh with given parameters

```lua
local radius           = 10 -- [mm]
local double height    = 30 -- [mm]
local int sector_count = 6  -- [count]

local mesh = make_frustum_dowel(radius, height, sector_count)
```


##### `api.make_pyramid`

Create pyramid mesh with given parameters

```lua
local base   = 10 -- [mm]
local height = 10

local mesh = make_pyramid(base, height)
```


##### `api.make_snap`

Create snap mesh with given parameters

```lua
local radius           = 10  -- [mm]
local height           = 20  -- [mm]
local space_proportion = 0.1 -- [ratio] optional, default 0.25
local bulge_proportion = 0.1 -- [ratio] optional, default 0.125

local mesh = make_snap(radius, height, space_proportion, bulge_proportion)
```


##### `api.make_torus`

Create torus mesh with given parameters

```lua
local r  = 123 -- [mm] main radius
local t  = 123 -- [mm] secondary radius
local ra = 123 -- [deg] optional, default 1
local ta = 123 -- [deg] optional, default 1

local mesh = make_torus(r, t, ra, ta)
```

##### `api.emboss_svg`

Emboss given svg (path)

```lua
local depth = 10 -- depth [mm]
local mesh = api.emboss_svg("drawing.svg", depth)

-- mesh: Mesh

```

##### `api.emboss_text`

Emboss given text

```lua
local mesh = api.emboss_text { 
  font=api.get_default_font(), 
  text="Hello world", 
  depth=1.0,          -- emboss depth [mm]
  line_height=12.0,   -- single text line height [mm] (default 10.0)  
}

-- mesh: Mesh

```

#### Classes

##### Class `Mesh`

A triangle mesh 

###### Method `Mesh:bounds()`

Gets bounding box of the mesh.

```lua
local bb = mesh:bounds()
-- bb: BoundingBox
```

##### Class `BoundingBox`

Read-only properties:
- `min_x`, `max_x` : double
- `min_y`, `max_y` : double
- `min_z`, `max_z` : double


```
local bb = mesh:bounds()
local mesh = api.make_cube(bb.max_x - bb.min_x, bb.max_y - bb.min_y,  bb.max_z - bb.min_z),
```

##### Enum `VolumeType`

Constants:
- `VolumeType.Solid`
- `VolumeType.Negative`
- `VolumeType.Modifier`
- `VolumeType.SupportBlocker`
- `VolumeType.SupportEnforcer`
- `VolumeType.Invalid`

##### Class `ProjectApi`

This is main entry point to the loaded project, the plugin is executed upon. 

An instance of this class is exposed as `api.project`.

###### Method `ProjectApi:add_object`

Adds object into the scene.

The method accepts single table argument with following structure:
- `mesh` (Mesh, required) First solid part geometry
- `other_volumes` (array, optional) List of other volumes to add, each item in that list can have the following keys:
  - `mesh` (Mesh, required) Volume geometry
  - `type` (VolumeType, required) Type of the volume
  - `translate` (table, optional) Translation of the volume with respect to the origin of object, keys `x`, `y`, or `z` 
    are expected (all optional).  
  - `rotate` (table, optional) Rotation of the volume with respect to the origin of object around given axis in degrees, keys `x`, `y`, or `z`
    are expected (all optional).
  - `params` (table, optional) Preset overrides for given object, a table with keys matching preset item name 
    (e.g. `perimeter_speed`) and value of appropriate type.

Example:

```lua
local base_mesh = api.emboss_svg("drawing.svg", 10)
local bb = base_mesh:bounds() 
local obj = api.project:add_object{
  mesh=base_mesh,
  other_volumes={
    {
      type=VolumeType.Modifier,
      mesh=api.make_cube(
        bb.max_x - bb.min_x, 
        bb.max_y - bb.min_y, 
        bb.max_z - bb.min_z
      ),
      translate={
        x=bb.min_x, y=bb.min_y, z=bb.min_z
      },
      params={
        perimeter_speed=40
      }
    }
  }
}

```