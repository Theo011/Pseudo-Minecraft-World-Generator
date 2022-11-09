# Pseudo Minecraft World Generator

Pseudo Minecraft world generator written in C++ using OpenGL 4.6. **This is NOT how Minecraft generates its worlds.**

Random generation is based on Perlin noise (but the type and parameters can be easily changed), world size can be controlled, as it is now it will only generate a single chunk of the preferred size. The type of lighting is Phong but specular has been removed because it looked weird on Minecraft's blocks. Instanced rendering is being utilized to reduce draw calls and improve performance.
