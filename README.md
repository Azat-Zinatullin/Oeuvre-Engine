# Oeuvre-Engine

My first game engine, which I am developing for educational and experimental purposes.

![image 2025-02-11 17-28-32-429](https://github.com/user-attachments/assets/1b159c25-60c4-4503-ae41-d6e9a416b278)

### Current features
- IBL PBR
- Deferred shading
- Omnidirectional shadows
- Cascaded shadows
- Physics (WIP)
- Tone mapping
- Frustum culling
- HDRI skyboxes
- Skeletal animation and basic character movement

### Controls
WASD: camera/character movement<br>
C: toggle viewport mode<br>
Q: toggle gizmo local/world mode<br>
W/E/R: select gizmo translate/rotate/scale operation<br>
F: throw a cube (physics demo)

### Download

The .lib and .dll files are managed by LFS. Download via terminal:

```
git lfs install
git clone https://github.com/JxhnStoner/Oeuvre-Engine.git
```

### Build

To create a Visual Studio 2022 solution run "GenerateProjects.bat".
