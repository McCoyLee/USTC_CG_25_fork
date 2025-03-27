#include "GCore/Components/MeshOperand.h"
#include "geom_node_base.h"
#include "GCore/util_openmesh_bind.h"
#include <Eigen/Sparse>
#include <cmath>

NODE_DEF_OPEN_SCOPE

// Circle boundary mapping declaration
NODE_DECLARATION_FUNCTION(circle_boundary_mapping)
{
    b.add_input<Geometry>("Input");
    b.add_output<Geometry>("Output");
}

// Circle boundary mapping implementation
NODE_EXECUTION_FUNCTION(circle_boundary_mapping)
{
    auto input = params.get_input<Geometry>("Input");
    if (!input.get_component<MeshComponent>()) {
        throw std::runtime_error("Boundary Mapping: Need Geometry Input.");
    }

    auto halfedge_mesh = operand_to_openmesh(&input);
    std::vector<OpenMesh::VertexHandle> boundary_vertices;

    // Find boundary vertices
    for (auto he_it = halfedge_mesh->halfedges_begin(); he_it != halfedge_mesh->halfedges_end(); ++he_it) {
        if (halfedge_mesh->is_boundary(*he_it)) {
            OpenMesh::HalfedgeHandle current_he = *he_it;
            do {
                boundary_vertices.push_back(halfedge_mesh->to_vertex_handle(current_he));
                current_he = halfedge_mesh->next_halfedge_handle(current_he);
            } while (current_he != *he_it);
            break;
        }
    }

    if (boundary_vertices.empty()) {
        throw std::runtime_error("Boundary Mapping: No boundary found.");
    }

    // Map boundary to circle
    const size_t n = boundary_vertices.size();
    for (size_t i = 0; i < n; ++i) {
        const double theta = 2.0 * M_PI * i / n;
        halfedge_mesh->set_point(boundary_vertices[i],
            OpenMesh::Vec3f(
                0.5f + 0.5f * static_cast<float>(cos(theta)),
                0.5f + 0.5f * static_cast<float>(sin(theta)),
                0.0f
            ));
    }

    auto geometry = openmesh_to_operand(halfedge_mesh.get());
    params.set_output("Output", std::move(*geometry));
    return true;
}

// Square boundary mapping declaration
NODE_DECLARATION_FUNCTION(square_boundary_mapping)
{
    b.add_input<Geometry>("Input");
    b.add_output<Geometry>("Output");
}

// Square boundary mapping implementation
NODE_EXECUTION_FUNCTION(square_boundary_mapping)
{
    auto input = params.get_input<Geometry>("Input");
    if (!input.get_component<MeshComponent>()) {
        throw std::runtime_error("Input does not contain a mesh");
    }

    auto halfedge_mesh = operand_to_openmesh(&input);
    std::vector<OpenMesh::VertexHandle> boundary_vertices;

    // Find boundary vertices
    for (auto he_it = halfedge_mesh->halfedges_begin(); he_it != halfedge_mesh->halfedges_end(); ++he_it) {
        if (halfedge_mesh->is_boundary(*he_it)) {
            OpenMesh::HalfedgeHandle current_he = *he_it;
            do {
                boundary_vertices.push_back(halfedge_mesh->to_vertex_handle(current_he));
                current_he = halfedge_mesh->next_halfedge_handle(current_he);
            } while (current_he != *he_it);
            break;
        }
    }

    // Map boundary to square
    const size_t n = boundary_vertices.size();
    for (size_t i = 0; i < n; ++i) {
        const double s = 4.0 * i / n;
        OpenMesh::Vec3f new_point(0.0f, 0.0f, 0.0f);

        if (s < 1.0) {
            new_point[0] = static_cast<float>(s);
        }
        else if (s < 2.0) {
            new_point[0] = 1.0f;
            new_point[1] = static_cast<float>(s - 1.0);
        }
        else if (s < 3.0) {
            new_point[0] = static_cast<float>(3.0 - s);
            new_point[1] = 1.0f;
        }
        else {
            new_point[1] = static_cast<float>(4.0 - s);
        }

        halfedge_mesh->set_point(boundary_vertices[i], new_point);
    }

    auto geometry = openmesh_to_operand(halfedge_mesh.get());
    params.set_output("Output", std::move(*geometry));
    return true;
}

// Helper function for computing cotangent weight
static double compute_cot_weight(const OpenMesh::Vec3f& a,
    const OpenMesh::Vec3f& b,
    const OpenMesh::Vec3f& c)
{
    const OpenMesh::Vec3f v1 = a - c;
    const OpenMesh::Vec3f v2 = b - c;
    const double dot = v1.dot(v2);
    const double cross = v1.cross(v2).norm();
    return (cross < 1e-8) ? 0.0 : (dot / cross);
}

// Compute edge weight for Laplacian matrix
static double compute_edge_weight(OpenMesh::PolyMesh_ArrayKernelT<OpenMesh::DefaultTraits>* mesh,
    OpenMesh::EdgeHandle eh)
{
    if (!mesh->is_valid_handle(eh)) return 0.0;

    double weight = 0.0;
    const auto add_face_contribution = [&](OpenMesh::HalfedgeHandle heh) {
        if (mesh->is_valid_handle(heh)) {
            const OpenMesh::VertexHandle v0 = mesh->from_vertex_handle(heh);
            const OpenMesh::VertexHandle v1 = mesh->to_vertex_handle(heh);
            const OpenMesh::VertexHandle v2 = mesh->to_vertex_handle(mesh->next_halfedge_handle(heh));
            weight += compute_cot_weight(mesh->point(v0), mesh->point(v2), mesh->point(v1));
        }
        };

    add_face_contribution(mesh->halfedge_handle(eh, 0));
    add_face_contribution(mesh->halfedge_handle(eh, 1));
    return weight;
}

// Harmonic circle mapping declaration
NODE_DECLARATION_FUNCTION(harmonic_circle_boundary_mapping)
{
    b.add_input<Geometry>("Input");
    b.add_output<Geometry>("Output");
}

// Harmonic circle mapping implementation
NODE_EXECUTION_FUNCTION(harmonic_circle_boundary_mapping)
{
    auto input = params.get_input<Geometry>("Input");
    if (!input.get_component<MeshComponent>()) {
        throw std::runtime_error("Need Geometry Input.");
    }

    auto halfedge_mesh = operand_to_openmesh(&input);
    std::vector<OpenMesh::VertexHandle> boundary_vertices;

    // Find boundary vertices
    for (auto he_it = halfedge_mesh->halfedges_begin(); he_it != halfedge_mesh->halfedges_end(); ++he_it) {
        if (halfedge_mesh->is_boundary(*he_it)) {
            OpenMesh::HalfedgeHandle current_he = *he_it;
            do {
                boundary_vertices.push_back(halfedge_mesh->to_vertex_handle(current_he));
                current_he = halfedge_mesh->next_halfedge_handle(current_he);
            } while (current_he != *he_it);
            break;
        }
    }

    // Map boundary to circle
    const size_t n = boundary_vertices.size();
    for (size_t i = 0; i < n; ++i) {
        const double theta = 2.0 * M_PI * i / n;
        halfedge_mesh->set_point(boundary_vertices[i],
            OpenMesh::Vec3f(
                0.5f + 0.5f * static_cast<float>(cos(theta)),
                0.5f + 0.5f * static_cast<float>(sin(theta)),
                0.0f
            ));
    }

    // Collect interior vertices
    std::vector<OpenMesh::VertexHandle> interior_vertices;
    std::map<OpenMesh::VertexHandle, int> vertex_index_map;

    int index = 0;
    for (auto v_it = halfedge_mesh->vertices_begin(); v_it != halfedge_mesh->vertices_end(); ++v_it) {
        if (std::find(boundary_vertices.begin(), boundary_vertices.end(), *v_it) == boundary_vertices.end()) {
            interior_vertices.push_back(*v_it);
            vertex_index_map[*v_it] = index++;
        }
    }

    if (!interior_vertices.empty()) {
        Eigen::SparseMatrix<double> laplacian(interior_vertices.size(), interior_vertices.size());
        std::vector<Eigen::Triplet<double>> triplets;

        // Build Laplacian matrix
        for (size_t i = 0; i < interior_vertices.size(); ++i) {
            const auto vh = interior_vertices[i];
            double diagonal = 0.0;

            for (auto vv_it = halfedge_mesh->vv_begin(vh); vv_it.is_valid(); ++vv_it) {
                const auto eh = halfedge_mesh->edge_handle(halfedge_mesh->find_halfedge(vh, *vv_it));
                const double weight = compute_edge_weight(halfedge_mesh.get(), eh);

                if (vertex_index_map.count(*vv_it)) {
                    triplets.emplace_back(i, vertex_index_map[*vv_it], -weight);
                    diagonal += weight;
                }
                else {
                    // Handle boundary vertices
                    diagonal += weight;
                }
            }

            triplets.emplace_back(i, i, diagonal);
        }

        laplacian.setFromTriplets(triplets.begin(), triplets.end());

        // Solve for x coordinates
        Eigen::VectorXd rhs_x = Eigen::VectorXd::Zero(interior_vertices.size());
        for (size_t i = 0; i < interior_vertices.size(); ++i) {
            const auto vh = interior_vertices[i];
            for (auto vv_it = halfedge_mesh->vv_begin(vh); vv_it.is_valid(); ++vv_it) {
                if (std::find(boundary_vertices.begin(), boundary_vertices.end(), *vv_it) != boundary_vertices.end()) {
                    const auto eh = halfedge_mesh->edge_handle(halfedge_mesh->find_halfedge(vh, *vv_it));
                    const double weight = compute_edge_weight(halfedge_mesh.get(), eh);
                    rhs_x[i] += weight * halfedge_mesh->point(*vv_it)[0];
                }
            }
        }

        // Solve for y coordinates
        Eigen::VectorXd rhs_y = Eigen::VectorXd::Zero(interior_vertices.size());
        for (size_t i = 0; i < interior_vertices.size(); ++i) {
            const auto vh = interior_vertices[i];
            for (auto vv_it = halfedge_mesh->vv_begin(vh); vv_it.is_valid(); ++vv_it) {
                if (std::find(boundary_vertices.begin(), boundary_vertices.end(), *vv_it) != boundary_vertices.end()) {
                    const auto eh = halfedge_mesh->edge_handle(halfedge_mesh->find_halfedge(vh, *vv_it));
                    const double weight = compute_edge_weight(halfedge_mesh.get(), eh);
                    rhs_y[i] += weight * halfedge_mesh->point(*vv_it)[1];
                }
            }
        }

        // Solve linear systems
        Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
        solver.compute(laplacian);

        if (solver.info() != Eigen::Success) {
            throw std::runtime_error("Matrix decomposition failed");
        }

        const Eigen::VectorXd x_sol = solver.solve(rhs_x);
        const Eigen::VectorXd y_sol = solver.solve(rhs_y);

        // Update interior vertices
        for (size_t i = 0; i < interior_vertices.size(); ++i) {
            halfedge_mesh->set_point(interior_vertices[i],
                OpenMesh::Vec3f(
                    static_cast<float>(x_sol[i]),
                    static_cast<float>(y_sol[i]),
                    0.0f
                ));
        }
    }

    auto geometry = openmesh_to_operand(halfedge_mesh.get());
    params.set_output("Output", std::move(*geometry));
    return true;
}

// Harmonic square mapping declaration
NODE_DECLARATION_FUNCTION(harmonic_square_boundary_mapping)
{
    b.add_input<Geometry>("Input");
    b.add_output<Geometry>("Output");
}

// Harmonic square mapping implementation
NODE_EXECUTION_FUNCTION(harmonic_square_boundary_mapping)
{
    auto input = params.get_input<Geometry>("Input");
    if (!input.get_component<MeshComponent>()) {
        throw std::runtime_error("Need Geometry Input.");
    }

    auto halfedge_mesh = operand_to_openmesh(&input);
    std::vector<OpenMesh::VertexHandle> boundary_vertices;

    // Find boundary vertices (same as square_boundary_mapping)
    for (auto he_it = halfedge_mesh->halfedges_begin(); he_it != halfedge_mesh->halfedges_end(); ++he_it) {
        if (halfedge_mesh->is_boundary(*he_it)) {
            OpenMesh::HalfedgeHandle current_he = *he_it;
            do {
                boundary_vertices.push_back(halfedge_mesh->to_vertex_handle(current_he));
                current_he = halfedge_mesh->next_halfedge_handle(current_he);
            } while (current_he != *he_it);
            break;
        }
    }

    // Map boundary to square (same as square_boundary_mapping)
    const size_t n = boundary_vertices.size();
    for (size_t i = 0; i < n; ++i) {
        const double s = 4.0 * i / n;
        OpenMesh::Vec3f new_point(0.0f, 0.0f, 0.0f);

        if (s < 1.0) {
            new_point[0] = static_cast<float>(s);
        }
        else if (s < 2.0) {
            new_point[0] = 1.0f;
            new_point[1] = static_cast<float>(s - 1.0);
        }
        else if (s < 3.0) {
            new_point[0] = static_cast<float>(3.0 - s);
            new_point[1] = 1.0f;
        }
        else {
            new_point[1] = static_cast<float>(4.0 - s);
        }

        halfedge_mesh->set_point(boundary_vertices[i], new_point);
    }

    // Collect interior vertices (same as harmonic_circle_boundary_mapping)
    std::vector<OpenMesh::VertexHandle> interior_vertices;
    std::map<OpenMesh::VertexHandle, int> vertex_index_map;

    int index = 0;
    for (auto v_it = halfedge_mesh->vertices_begin(); v_it != halfedge_mesh->vertices_end(); ++v_it) {
        if (std::find(boundary_vertices.begin(), boundary_vertices.end(), *v_it) == boundary_vertices.end()) {
            interior_vertices.push_back(*v_it);
            vertex_index_map[*v_it] = index++;
        }
    }

    if (!interior_vertices.empty()) {
        Eigen::SparseMatrix<double> laplacian(interior_vertices.size(), interior_vertices.size());
        std::vector<Eigen::Triplet<double>> triplets;

        // Build Laplacian matrix (same as harmonic_circle_boundary_mapping)
        for (size_t i = 0; i < interior_vertices.size(); ++i) {
            const auto vh = interior_vertices[i];
            double diagonal = 0.0;

            for (auto vv_it = halfedge_mesh->vv_begin(vh); vv_it.is_valid(); ++vv_it) {
                const auto eh = halfedge_mesh->edge_handle(halfedge_mesh->find_halfedge(vh, *vv_it));
                const double weight = compute_edge_weight(halfedge_mesh.get(), eh);

                if (vertex_index_map.count(*vv_it)) {
                    triplets.emplace_back(i, vertex_index_map[*vv_it], -weight);
                    diagonal += weight;
                }
                else {
                    // Handle boundary vertices
                    diagonal += weight;
                }
            }

            triplets.emplace_back(i, i, diagonal);
        }

        laplacian.setFromTriplets(triplets.begin(), triplets.end());

        // Solve for x coordinates (same as harmonic_circle_boundary_mapping)
        Eigen::VectorXd rhs_x = Eigen::VectorXd::Zero(interior_vertices.size());
        for (size_t i = 0; i < interior_vertices.size(); ++i) {
            const auto vh = interior_vertices[i];
            for (auto vv_it = halfedge_mesh->vv_begin(vh); vv_it.is_valid(); ++vv_it) {
                if (std::find(boundary_vertices.begin(), boundary_vertices.end(), *vv_it) != boundary_vertices.end()) {
                    const auto eh = halfedge_mesh->edge_handle(halfedge_mesh->find_halfedge(vh, *vv_it));
                    const double weight = compute_edge_weight(halfedge_mesh.get(), eh);
                    rhs_x[i] += weight * halfedge_mesh->point(*vv_it)[0];
                }
            }
        }

        // Solve for y coordinates (same as harmonic_circle_boundary_mapping)
        Eigen::VectorXd rhs_y = Eigen::VectorXd::Zero(interior_vertices.size());
        for (size_t i = 0; i < interior_vertices.size(); ++i) {
            const auto vh = interior_vertices[i];
            for (auto vv_it = halfedge_mesh->vv_begin(vh); vv_it.is_valid(); ++vv_it) {
                if (std::find(boundary_vertices.begin(), boundary_vertices.end(), *vv_it) != boundary_vertices.end()) {
                    const auto eh = halfedge_mesh->edge_handle(halfedge_mesh->find_halfedge(vh, *vv_it));
                    const double weight = compute_edge_weight(halfedge_mesh.get(), eh);
                    rhs_y[i] += weight * halfedge_mesh->point(*vv_it)[1];
                }
            }
        }

        // Solve linear systems (same as harmonic_circle_boundary_mapping)
        Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
        solver.compute(laplacian);

        if (solver.info() != Eigen::Success) {
            throw std::runtime_error("Matrix decomposition failed");
        }

        const Eigen::VectorXd x_sol = solver.solve(rhs_x);
        const Eigen::VectorXd y_sol = solver.solve(rhs_y);

        // Update interior vertices (same as harmonic_circle_boundary_mapping)
        for (size_t i = 0; i < interior_vertices.size(); ++i) {
            halfedge_mesh->set_point(interior_vertices[i],
                OpenMesh::Vec3f(
                    static_cast<float>(x_sol[i]),
                    static_cast<float>(y_sol[i]),
                    0.0f
                ));
        }
    }

    auto geometry = openmesh_to_operand(halfedge_mesh.get());
    params.set_output("Output", std::move(*geometry));
    return true;
}

NODE_DECLARATION_UI(boundary_mapping);
NODE_DEF_CLOSE_SCOPE