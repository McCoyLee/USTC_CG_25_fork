#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "GCore/Components/MeshOperand.h"
#include "GCore/GOP.h"
#include "nodes/core/def/node_def.hpp"
#include "Eigen/Dense"

#include "geom_node_base.h"

typedef OpenMesh::TriMesh_ArrayKernelT<> MyMesh;

#include <queue>
#include <unordered_map>

bool find_shortest_path(
    const MyMesh::VertexHandle& start_vertex_handle,
    const MyMesh::VertexHandle& end_vertex_handle,
    const MyMesh& omesh,
    std::list<size_t>& shortest_path_vertex_indices,
    float& distance)
{
    std::unordered_map<MyMesh::VertexHandle, float> distances;
    std::unordered_map<MyMesh::VertexHandle, MyMesh::VertexHandle> predecessors;

    auto cmp = [](const std::pair<float, MyMesh::VertexHandle>& a,
        const std::pair<float, MyMesh::VertexHandle>& b) {
            return a.first > b.first;
        };

    std::priority_queue<std::pair<float, MyMesh::VertexHandle>,
        std::vector<std::pair<float, MyMesh::VertexHandle>>,
        decltype(cmp)> queue(cmp);

    distances[start_vertex_handle] = 0.0f;
    queue.push({ 0.0f, start_vertex_handle });

    while (!queue.empty()) {
        auto current = queue.top();
        queue.pop();

        float current_distance = current.first;
        MyMesh::VertexHandle u = current.second;

        if (u == end_vertex_handle) break;
        if (current_distance > distances[u]) continue;

        for (MyMesh::ConstVertexVertexIter vv_it = omesh.cvv_iter(u);
            vv_it.is_valid();
            ++vv_it)
        {
            MyMesh::VertexHandle v = *vv_it;

            const MyMesh::Point& u_pos = omesh.point(u);
            const MyMesh::Point& v_pos = omesh.point(v);
            float edge_weight = (u_pos - v_pos).norm();

            float new_dist = distances[u] + edge_weight;
            if (!distances.count(v) || new_dist < distances[v]) {
                distances[v] = new_dist;
                predecessors[v] = u;
                queue.push({ new_dist, v });
            }
        }
    }

    if (!predecessors.count(end_vertex_handle)) {
        return false;
    }

    MyMesh::VertexHandle current = end_vertex_handle;
    while (current != start_vertex_handle) {
        shortest_path_vertex_indices.push_front(current.idx());
        current = predecessors[current];
    }
    shortest_path_vertex_indices.push_front(start_vertex_handle.idx());

    distance = distances[end_vertex_handle];
    return true;
}

NODE_DEF_OPEN_SCOPE

NODE_DECLARATION_FUNCTION(assignments_shortest_path)
{
    b.add_input<std::string>("Picked Mesh [0] Name");
    b.add_input<std::string>("Picked Mesh [1] Name");
    b.add_input<Geometry>("Picked Mesh");
    b.add_input<size_t>("Picked Vertex [0] Index");
    b.add_input<size_t>("Picked Vertex [1] Index");

    b.add_output<std::list<size_t>>("Shortest Path Vertex Indices");
    b.add_output<float>("Shortest Path Distance");
}

NODE_EXECUTION_FUNCTION(assignments_shortest_path)
{

    auto picked_mesh_0_name =
        params.get_input<std::string>("Picked Mesh [0] Name");
    auto picked_mesh_1_name =
        params.get_input<std::string>("Picked Mesh [1] Name");
    // Ensure that the two picked meshes are the same
    if (picked_mesh_0_name != picked_mesh_1_name) {
        std::cerr << "Ensure that the two picked meshes are the same"
                  << std::endl;
        return false;
    }

    auto mesh = params.get_input<Geometry>("Picked Mesh")
                    .get_component<MeshComponent>();
    auto vertices = mesh->get_vertices();
    auto face_vertex_counts = mesh->get_face_vertex_counts();
    auto face_vertex_indices = mesh->get_face_vertex_indices();

    // Convert the mesh to OpenMesh
    MyMesh omesh;
    // Add vertices
    std::vector<OpenMesh::VertexHandle> vhandles;
    for (size_t i = 0; i < vertices.size(); i++) {
        omesh.add_vertex(
            OpenMesh::Vec3f(vertices[i][0], vertices[i][1], vertices[i][2]));
    }
    // Add faces
    size_t start = 0;
    for (size_t i = 0; i < face_vertex_counts.size(); i++) {
        std::vector<OpenMesh::VertexHandle> face;
        for (size_t j = 0; j < face_vertex_counts[i]; j++) {
            face.push_back(
                OpenMesh::VertexHandle(face_vertex_indices[start + j]));
        }
        omesh.add_face(face);
        start += face_vertex_counts[i];
    }

    auto start_vertex_index =
        params.get_input<size_t>("Picked Vertex [0] Index");
    auto end_vertex_index = params.get_input<size_t>("Picked Vertex [1] Index");

    // Turn the vertex indices into OpenMesh vertex handles
    OpenMesh::VertexHandle start_vertex_handle(start_vertex_index);
    OpenMesh::VertexHandle end_vertex_handle(end_vertex_index);

    // The indices of the vertices on the shortest path, including the start and
    // end vertices
    std::list<size_t> shortest_path_vertex_indices;

    // The distance of the shortest path
    float distance = 0.0f;

    if (find_shortest_path(
            start_vertex_handle,
            end_vertex_handle,
            omesh,
            shortest_path_vertex_indices,
            distance)) {
        params.set_output(
            "Shortest Path Vertex Indices", shortest_path_vertex_indices);
        params.set_output("Shortest Path Distance", distance);
        return true;
    }
    else {
        params.set_output("Shortest Path Vertex Indices", std::list<size_t>());
        params.set_output("Shortest Path Distance", 0.0f);
        return false;
    }

    return true;
}

NODE_DECLARATION_UI(assignments_shortest_path);
NODE_DECLARATION_REQUIRED(assignments_shortest_path);

NODE_DEF_CLOSE_SCOPE