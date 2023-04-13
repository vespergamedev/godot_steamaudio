/******************************************************************************
MIT License

Copyright (c) 2023 saturnian-tides

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#ifndef STEAMAUDIO_GEOMETRY_H
#define STEAMAUDIO_GEOMETRY_H

#include "scene/3d/node_3d.h"
#include "steamaudio_server.h"
#include "scene/3d/mesh_instance_3d.h"
class SteamAudioGeometry : public Node3D {
    GDCLASS(SteamAudioGeometry, Node3D);
public:
    SteamAudioGeometry();
    ~SteamAudioGeometry();
    int register_geometry();
    int deregister_geometry();
    int create_geometry(const Ref<Mesh> mesh, Transform3D mesh_global_transform);
    int destroy_geometry();
    static void _bind_methods();
private:
    GlobalStateSteamAudio * global_state = nullptr;
    Vector<IPLStaticMesh> static_meshes;
};


#endif // STEAMAUDIO_GEOMETRY_H
