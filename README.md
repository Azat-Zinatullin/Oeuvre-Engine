# Oeuvre-Engine

This is a small game engine that I am developing for educational purposes. Feel free to use the techniques from my code in your own projects.

<img width="1477" height="1226" alt="Screenshot 2025-10-29 175518" src="https://github.com/user-attachments/assets/fd6d0390-17f3-4247-b5c6-6e228f4505cc" />

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
