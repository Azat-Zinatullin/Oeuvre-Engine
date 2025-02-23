# Oeuvre-Engine

This is a test game engine that I am developing for educational purposes. Feel free to use the techniques from my code in your own projects.

![image 2025-02-11 17-28-32-429](https://github.com/user-attachments/assets/1b159c25-60c4-4503-ae41-d6e9a416b278)

### Current features
- Voxel cone tracing global illumination (NVIDIA VXGI)
- NVIDIA PhysX (integration in process)
- PBR materials support
- Deferred shading
- Tone mapping
- Frustum culling
- HDRI cubemaps support
- Omnidirectional shadowmaps
- Skeletal animation and basic character movement

### Controls
WASD: camera/character movement<br>
C: toggle viewport mode<br>

### Download

The .lib and .dll files are managed by LFS. Download via terminal:

```
git lfs install
git clone https://github.com/JxhnStoner/Oeuvre-Engine.git
```

### Build

To create a Visual Studio 2022 solution run "GenerateProjects.bat".
