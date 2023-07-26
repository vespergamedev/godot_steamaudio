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

#include "steamaudio_geometry.h"
#include <godot_cpp/core/class_db.hpp>

SteamAudioGeometry::SteamAudioGeometry() {
    global_state = SteamAudioServer::get_singleton()->clone_global_state();
}

SteamAudioGeometry::~SteamAudioGeometry() {
    deregister_geometry();
    destroy_geometry();
}

int SteamAudioGeometry::create_geometry(const Ref<Mesh> mesh, Transform3D mesh_global_transform) {
    int n_surfaces = mesh->get_surface_count();
    IPLMaterial default_replace_me;
    default_replace_me.absorption[0] = 0.10f;
    default_replace_me.absorption[1] = 0.20f;
    default_replace_me.absorption[2] = 0.30f;
    default_replace_me.scattering = 0.05f;
    default_replace_me.transmission[0] = 0.100f;
    default_replace_me.transmission[1] = 0.050f;
    default_replace_me.transmission[2] = 0.030f;
     
    for (int sidx = 0; sidx < n_surfaces; sidx++) {
        Array surface_data = mesh->surface_get_arrays(sidx);
        Array verts_gd = surface_data[Mesh::ARRAY_VERTEX];
        Array triangles_gd = surface_data[Mesh::ARRAY_INDEX];
        Vector<IPLVector3> verts;
        Vector<IPLTriangle> triangles;
        Vector<IPLint32> material_indices;
        verts.resize(verts_gd.size());
        for (int vidx = 0; vidx < verts_gd.size(); vidx++) {
            IPLVector3 vert;
            Vector3 vert_gd = verts_gd[vidx];
            vert_gd = mesh_global_transform.basis.xform(vert_gd);
            vert_gd += mesh_global_transform.origin;
            vert.x = vert_gd.x;
            vert.y = vert_gd.y;
            vert.z = vert_gd.z;
            
            verts.set(vidx, vert);

        }
        for (int tidx = 0; tidx < triangles_gd.size(); tidx+=3) {
            IPLTriangle tri;
            //Convert Godot CW to SteamAudio CCW
            tri.indices[0] = triangles_gd[tidx];
            tri.indices[1] = triangles_gd[tidx+2];
            tri.indices[2] = triangles_gd[tidx+1];
            triangles.push_back(tri);
            material_indices.push_back(0);
        }

        IPLStaticMeshSettings static_mesh_settings{};
        static_mesh_settings.numVertices = verts.size();
        static_mesh_settings.numTriangles = triangles.size();
        static_mesh_settings.numMaterials = 1;
        static_mesh_settings.vertices = verts.ptrw();
        static_mesh_settings.triangles =  triangles.ptrw();
        static_mesh_settings.materialIndices = material_indices.ptrw();
        static_mesh_settings.materials = &default_replace_me;
        IPLStaticMesh static_mesh = nullptr;
        IPLerror errorCode = iplStaticMeshCreate(global_state->scene, &static_mesh_settings, &static_mesh);
        if (errorCode) {
            printf("Err code for iplStaticMeshCreate: %d\n", errorCode);
            return (int)errorCode;
        }

        static_meshes.push_back(iplStaticMeshRetain(static_mesh));
    }
    return 0;
    
}

int SteamAudioGeometry::destroy_geometry() {
    for (int midx = 0; midx < static_meshes.size(); midx++) {
        IPLStaticMesh mesh_ptr = static_meshes.get(midx);
        iplStaticMeshRelease(&mesh_ptr);
    }
    static_meshes.clear();
    return 0;
}

int SteamAudioGeometry::register_geometry() {
    for (int midx = 0; midx < static_meshes.size(); midx++) {
        iplStaticMeshAdd(static_meshes.get(midx), global_state->scene);
    }
    return 0;
}

int SteamAudioGeometry::deregister_geometry() {
    for (int midx = 0; midx < static_meshes.size(); midx++) {
        iplStaticMeshRemove(static_meshes.get(midx), global_state->scene);
    }
    return 0;
}

void SteamAudioGeometry::_bind_methods() {
	ClassDB::bind_method(D_METHOD("create_geometry", "mesh", "mesh_global_transform"), &SteamAudioGeometry::create_geometry);
	ClassDB::bind_method(D_METHOD("destroy_geometry"), &SteamAudioGeometry::destroy_geometry);

	ClassDB::bind_method(D_METHOD("register_geometry"), &SteamAudioGeometry::register_geometry);
	ClassDB::bind_method(D_METHOD("deregister_geometry"), &SteamAudioGeometry::deregister_geometry);

}

