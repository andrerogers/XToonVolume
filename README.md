# Extended Toon Voxel Rendering

A GPU based voxel rendering of MRI data, which is lit based on my earlier extended too implementation (https://github.com/andrerogers/XToonShader) but modified to work with volumetric data.

Youtube Video:

https://www.youtube.com/watch?v=lovKtK9_ygs

https://www.youtube.com/watch?v=_J5LxszzhnE

## 2026 rebuild: animated sparse voxel octrees

[`svo/`](svo/) is a rebuild that does what the thesis set out to do: animated
sparse voxel octrees with X-Toon shading, as a C++20/CMake project with an
OpenGL 4.5 compute renderer for Linux. It loads the original volumes and toon
textures and fixes the shading bugs in the 2016 shaders. See
[`svo/README.md`](svo/README.md) for how to build it and how it works.

The original Visual Studio project in `VolRender/` is unchanged.
