#include "GCore/Components/MeshOperand.h"
#include "geom_node_base.h"
#include "GCore/util_openmesh_bind.h"
#include <Eigen/Sparse>
#include <cmath>
#include <vector>
#include <map>
#include <stdexcept>
#include <algorithm>

//-----------------------------------------------------------------------------
// Helper functions for cotangent weight computation
//-----------------------------------------------------------------------------

// Compute the cotangent of the angle at vertex 'c', given triangle vertices a, b, c.
static double compute_cot_weight(const OpenMesh::Vec3f& a,
    const OpenMesh::Vec3f& b,
    const OpenMesh::Vec3f& c)
{
    OpenMesh::Vec3f v1 = a - c;
    OpenMesh::Vec3f v2 = b - c;
    double dot = v1.dot(v2);
    double cross_norm = (v1 % v2).norm(); // '%' denotes cross product
    return (cross_norm < 1e-8) ? 0.0 : (dot / cross_norm);
}

// Compute the cotangent weight for the edge between vertices v and u.
// For an interior edge, the weight is the sum of the cotangents of the two angles
// opposite to the edge. For a boundary edge, only one angle is used.
static double compute_edge_cot_weight(OpenMesh::PolyMesh_ArrayKernelT<OpenMesh::DefaultTraits>* mesh,
    OpenMesh::VertexHandle v,
    OpenMesh::VertexHandle u)
{
    double weight = 0.0;
    OpenMesh::HalfedgeHandle he = mesh->find_halfedge(v, u);
    if (he.is_valid()) {
        // Face on one side.
        if (!mesh->is_boundary(he)) {
            OpenMesh::HalfedgeHandle he_next = mesh->next_halfedge_handle(he);
            OpenMesh::VertexHandle opp = mesh->to_vertex_handle(he_next);
            weight += compute_cot_weight(mesh->point(v), mesh->point(u), mesh->point(opp));
        }
        // Face on the opposite side.
        OpenMesh::HalfedgeHandle he_opp = mesh->opposite_halfedge_handle(he);
        if (he_opp.is_valid() && !mesh->is_boundary(he_opp)) {
            OpenMesh::HalfedgeHandle he_opp_next = mesh->next_halfedge_handle(he_opp);
            OpenMesh::VertexHandle opp = mesh->to_vertex_handle(he_opp_next);
            weight += compute_cot_weight(mesh->point(v), mesh->point(u), mesh->point(opp));
        }
    }
    return weight;
}

//=============================================================================
// Laplacian Surface Editing with Cotangent Weight Node
//=============================================================================

NODE_DEF_OPEN_SCOPE

NODE_DECLARATION_FUNCTION(laplacian_surface_editing_cot)
{
    // Inputs: Original mesh, Changed vertices, Control Points Indices
    b.add_input<Geometry>("Original mesh");
    b.add_input<pxr::VtVec3fArray>("Changed vertices");
    b.add_input<std::vector<size_t>>("Control Points Indices");
    // Output: Edited mesh (Geometry)
    b.add_output<Geometry>("Output");
}

NODE_EXECUTION_FUNCTION(laplacian_surface_editing_cot)
{
    auto input = params.get_input<Geometry>("Original mesh");
    if (!input.get_component<MeshComponent>()) {
        throw std::runtime_error("Laplacian Surface Editing Cotangent: Need Geometry Input.");
    }
    auto halfedge_mesh = operand_to_openmesh(&input);

    std::vector<size_t> control_indices = params.get_input<std::vector<size_t>>("Control Points Indices");
    if (control_indices.empty()) {
        throw std::runtime_error("Laplacian Surface Editing Cotangent: At least one control point is required.");
    }
    std::vector<bool> is_control(halfedge_mesh->n_vertices(), false);
    for (size_t idx : control_indices) {
        if (idx < halfedge_mesh->n_vertices())
            is_control[idx] = true;
    }
    std::vector<OpenMesh::VertexHandle> interior_vertices;
    std::map<OpenMesh::VertexHandle, int> vertex_index_map;
    int interior_index = 0;
    for (auto v_it = halfedge_mesh->vertices_begin(); v_it != halfedge_mesh->vertices_end(); ++v_it) {
        if (!is_control[v_it->idx()]) {
            interior_vertices.push_back(*v_it);
            vertex_index_map[*v_it] = interior_index++;
        }
    }
    const int num_interior = interior_vertices.size();

    Eigen::SparseMatrix<double> L(num_interior, num_interior);
    std::vector<Eigen::Triplet<double>> triplets;
    Eigen::VectorXd bx = Eigen::VectorXd::Zero(num_interior);
    Eigen::VectorXd by = Eigen::VectorXd::Zero(num_interior);
    Eigen::VectorXd bz = Eigen::VectorXd::Zero(num_interior);

    auto changed_vertices = params.get_input<pxr::VtVec3fArray>("Changed vertices");

    for (int i = 0; i < num_interior; ++i) {
        OpenMesh::VertexHandle vh = interior_vertices[i];
        OpenMesh::Vec3f orig_pos = halfedge_mesh->point(vh);

        double sum_weights = 0.0;
        OpenMesh::Vec3f weighted_sum_orig(0.0f, 0.0f, 0.0f);
        OpenMesh::Vec3f weighted_sum_control(0.0f, 0.0f, 0.0f);
        std::vector<std::pair<int, double>> neighbor_weights;

        for (auto vv_it = halfedge_mesh->vv_begin(vh); vv_it.is_valid(); ++vv_it) {
            double w = compute_edge_cot_weight(halfedge_mesh.get(), vh, *vv_it);
            sum_weights += w;
            weighted_sum_orig += w * halfedge_mesh->point(*vv_it);
            if (is_control[(*vv_it).idx()]) {
                weighted_sum_control += w * OpenMesh::Vec3f(changed_vertices[(*vv_it).idx()][0],
                    changed_vertices[(*vv_it).idx()][1],
                    changed_vertices[(*vv_it).idx()][2]);
            }
            else {
                if (vertex_index_map.find(*vv_it) != vertex_index_map.end()) {
                    int j = vertex_index_map[*vv_it];
                    neighbor_weights.push_back(std::make_pair(j, w));
                }
            }
        }
        if (sum_weights == 0.0)
            sum_weights = 1e-8; // avoid division by zero

        double delta_x = orig_pos[0] - (weighted_sum_orig[0] / sum_weights);
        double delta_y = orig_pos[1] - (weighted_sum_orig[1] / sum_weights);
        double delta_z = orig_pos[2] - (weighted_sum_orig[2] / sum_weights);
        double control_contrib_x = weighted_sum_control[0] / sum_weights;
        double control_contrib_y = weighted_sum_control[1] / sum_weights;
        double control_contrib_z = weighted_sum_control[2] / sum_weights;

        bx[i] = delta_x + control_contrib_x;
        by[i] = delta_y + control_contrib_y;
        bz[i] = delta_z + control_contrib_z;

        triplets.push_back(Eigen::Triplet<double>(i, i, 1.0));
        for (auto& p : neighbor_weights) {
            int j = p.first;
            double w = p.second;
            double coef = w / sum_weights;
            triplets.push_back(Eigen::Triplet<double>(i, j, -coef));
        }
    }
    L.setFromTriplets(triplets.begin(), triplets.end());

    Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
    solver.compute(L);
    if (solver.info() != Eigen::Success) {
        throw std::runtime_error("Laplacian Surface Editing Cotangent: Matrix decomposition failed.");
    }
    Eigen::VectorXd sol_x = solver.solve(bx);
    Eigen::VectorXd sol_y = solver.solve(by);
    Eigen::VectorXd sol_z = solver.solve(bz);

    for (int i = 0; i < num_interior; ++i) {
        OpenMesh::VertexHandle vh = interior_vertices[i];
        halfedge_mesh->set_point(vh, OpenMesh::Vec3f(static_cast<float>(sol_x[i]),
            static_cast<float>(sol_y[i]),
            static_cast<float>(sol_z[i])));
    }
    for (size_t i = 0; i < halfedge_mesh->n_vertices(); ++i) {
        if (is_control[i]) {
            halfedge_mesh->set_point(OpenMesh::VertexHandle(static_cast<int>(i)),
                OpenMesh::Vec3f(changed_vertices[i][0],
                    changed_vertices[i][1],
                    changed_vertices[i][2]));
        }
    }

    auto geometry = openmesh_to_operand(halfedge_mesh.get());
    params.set_output("Output", std::move(*geometry));
    return true;
}

//=============================================================================
// Laplacian Surface Editing with Uniform Weight Node
//=============================================================================

NODE_DECLARATION_FUNCTION(laplacian_surface_editing)
{
    // Inputs: Original mesh, Changed vertices, Control Points Indices
    b.add_input<Geometry>("Original mesh");
    b.add_input<pxr::VtVec3fArray>("Changed vertices");
    b.add_input<std::vector<size_t>>("Control Points Indices");
    // Output: Edited mesh (Geometry)
    b.add_output<Geometry>("Output");
}

NODE_EXECUTION_FUNCTION(laplacian_surface_editing)
{
    auto input = params.get_input<Geometry>("Original mesh");
    if (!input.get_component<MeshComponent>()) {
        throw std::runtime_error("Laplacian Surface Editing: Need Geometry Input.");
    }
    auto halfedge_mesh = operand_to_openmesh(&input);

    std::vector<size_t> control_indices = params.get_input<std::vector<size_t>>("Control Points Indices");
    if (control_indices.empty()) {
        throw std::runtime_error("Laplacian Surface Editing: At least one control point is required.");
    }
    std::vector<bool> is_control(halfedge_mesh->n_vertices(), false);
    for (size_t idx : control_indices) {
        if (idx < halfedge_mesh->n_vertices())
            is_control[idx] = true;
    }
    std::vector<OpenMesh::VertexHandle> interior_vertices;
    std::map<OpenMesh::VertexHandle, int> vertex_index_map;
    int interior_index = 0;
    for (auto v_it = halfedge_mesh->vertices_begin(); v_it != halfedge_mesh->vertices_end(); ++v_it) {
        if (!is_control[v_it->idx()]) {
            interior_vertices.push_back(*v_it);
            vertex_index_map[*v_it] = interior_index++;
        }
    }
    const int num_interior = interior_vertices.size();

    Eigen::SparseMatrix<double> L(num_interior, num_interior);
    std::vector<Eigen::Triplet<double>> triplets;
    Eigen::VectorXd bx = Eigen::VectorXd::Zero(num_interior);
    Eigen::VectorXd by = Eigen::VectorXd::Zero(num_interior);
    Eigen::VectorXd bz = Eigen::VectorXd::Zero(num_interior);

    auto changed_vertices = params.get_input<pxr::VtVec3fArray>("Changed vertices");

    for (int i = 0; i < num_interior; ++i) {
        OpenMesh::VertexHandle vh = interior_vertices[i];
        OpenMesh::Vec3f orig_pos = halfedge_mesh->point(vh);

        int degree = 0;
        OpenMesh::Vec3f sum_orig(0.0f, 0.0f, 0.0f);
        OpenMesh::Vec3f sum_control(0.0f, 0.0f, 0.0f);

        for (auto vv_it = halfedge_mesh->vv_begin(vh); vv_it.is_valid(); ++vv_it) {
            ++degree;
            sum_orig += halfedge_mesh->point(*vv_it);
            if (is_control[(*vv_it).idx()]) {
                sum_control += OpenMesh::Vec3f(changed_vertices[(*vv_it).idx()][0],
                    changed_vertices[(*vv_it).idx()][1],
                    changed_vertices[(*vv_it).idx()][2]);
            }
        }
        if (degree == 0) {
            triplets.push_back(Eigen::Triplet<double>(i, i, 1.0));
            bx[i] = orig_pos[0];
            by[i] = orig_pos[1];
            bz[i] = orig_pos[2];
            continue;
        }
        double inv_degree = 1.0 / degree;
        double delta_x = orig_pos[0] - inv_degree * sum_orig[0];
        double delta_y = orig_pos[1] - inv_degree * sum_orig[1];
        double delta_z = orig_pos[2] - inv_degree * sum_orig[2];
        double control_contrib_x = inv_degree * sum_control[0];
        double control_contrib_y = inv_degree * sum_control[1];
        double control_contrib_z = inv_degree * sum_control[2];

        bx[i] = delta_x + control_contrib_x;
        by[i] = delta_y + control_contrib_y;
        bz[i] = delta_z + control_contrib_z;

        triplets.push_back(Eigen::Triplet<double>(i, i, 1.0));
        for (auto vv_it = halfedge_mesh->vv_begin(vh); vv_it.is_valid(); ++vv_it) {
            if (!is_control[(*vv_it).idx()]) {
                int j = vertex_index_map[*vv_it];
                triplets.push_back(Eigen::Triplet<double>(i, j, -inv_degree));
            }
        }
    }
    L.setFromTriplets(triplets.begin(), triplets.end());

    Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
    solver.compute(L);
    if (solver.info() != Eigen::Success) {
        throw std::runtime_error("Laplacian Surface Editing: Matrix decomposition failed.");
    }
    Eigen::VectorXd sol_x = solver.solve(bx);
    Eigen::VectorXd sol_y = solver.solve(by);
    Eigen::VectorXd sol_z = solver.solve(bz);

    for (int i = 0; i < num_interior; ++i) {
        OpenMesh::VertexHandle vh = interior_vertices[i];
        halfedge_mesh->set_point(vh, OpenMesh::Vec3f(static_cast<float>(sol_x[i]),
            static_cast<float>(sol_y[i]),
            static_cast<float>(sol_z[i])));
    }
    for (size_t i = 0; i < halfedge_mesh->n_vertices(); ++i) {
        if (is_control[i]) {
            halfedge_mesh->set_point(OpenMesh::VertexHandle(static_cast<int>(i)),
                OpenMesh::Vec3f(changed_vertices[i][0],
                    changed_vertices[i][1],
                    changed_vertices[i][2]));
        }
    }

    auto geometry = openmesh_to_operand(halfedge_mesh.get());
    params.set_output("Output", std::move(*geometry));
    return true;
}

NODE_DECLARATION_UI(laplacian_surface_editing);
NODE_DEF_CLOSE_SCOPE
