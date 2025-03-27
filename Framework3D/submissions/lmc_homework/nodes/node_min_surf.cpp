#include "GCore/Components/MeshOperand.h"
#include "GCore/util_openmesh_bind.h"
#include "geom_node_base.h"
#include <cmath>
#include <time.h>
#include <Eigen/Sparse>

NODE_DEF_OPEN_SCOPE
NODE_DECLARATION_FUNCTION(min_surf)
{
    b.add_input<Geometry>("Input");

    b.add_output<Geometry>("Output");
}

NODE_EXECUTION_FUNCTION(min_surf)
{
    // Get the input from params
    auto input = params.get_input<Geometry>("Input");

    // (TO BE UPDATED) Avoid processing the node when there is no input
    if (!input.get_component<MeshComponent>()) {
        throw std::runtime_error("Minimal Surface: Need Geometry Input.");
        return false;
    }

    auto halfedge_mesh = operand_to_openmesh(&input);

    std::vector<OpenMesh::VertexHandle> internal_vts, boundary_vts;
    for (auto vh : halfedge_mesh->vertices()) {
        if (halfedge_mesh->is_boundary(vh)) {
            boundary_vts.push_back(vh);
        }
        else {
            internal_vts.push_back(vh);
        }
    }

    if (internal_vts.empty()) {
        auto geometry = openmesh_to_operand(halfedge_mesh.get());
        params.set_output("Output", std::move(*geometry));
        return true;
    }

    std::unordered_map<OpenMesh::VertexHandle, int> vh_to_index;
    for (int i = 0; i < internal_vts.size(); ++i) {
        vh_to_index[internal_vts[i]] = i;
    }

    // Construct the Laplacian matrix
    std::vector<Eigen::Triplet<double>> tripletList;
    Eigen::VectorXd b_x(internal_vts.size()), b_y(internal_vts.size()),
        b_z(internal_vts.size());
    b_x.setZero();
    b_y.setZero();
    b_z.setZero();

    for (int i = 0; i < internal_vts.size(); ++i) {
        OpenMesh::VertexHandle vh_i = internal_vts[i];
        int degree = 0;
        double sum_x = 0.0;
        double sum_y = 0.0;
        double sum_z = 0.0;

        for (auto vv_it = halfedge_mesh->vv_begin(vh_i); vv_it.is_valid();
             ++vv_it) {
            OpenMesh::VertexHandle vh_j = *vv_it;

        if (halfedge_mesh->is_boundary(vh_j)) {
            auto pt = halfedge_mesh->point(vh_j);
            sum_x += pt[0];
            sum_y += pt[1];
            sum_z += pt[2];
        }
        else {
            auto it = vh_to_index.find(vh_j);
            if (it != vh_to_index.end()) {
                tripletList.emplace_back(i, it->second, -1.0);
            }
        }
        degree++;
    }

    tripletList.emplace_back(i, i, degree);

    b_x(i) = sum_x;
    b_y(i) = sum_y;
    b_z(i) = sum_z;
}

// Coustruct a sparse matrix
Eigen::SparseMatrix<double> L(internal_vts.size(), internal_vts.size());
L.setFromTriplets(tripletList.begin(), tripletList.end());

// Solve the linear system
Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
solver.compute(L);
if (solver.info() != Eigen::Success) {
    throw std::runtime_error("Linear system solving failed");
}

Eigen::VectorXd x_coords = solver.solve(b_x);
Eigen::VectorXd y_coords = solver.solve(b_y);
Eigen::VectorXd z_coords = solver.solve(b_z);
if (solver.info() != Eigen::Success) {
    throw std::runtime_error("Linear system solving failed");
}

// Update the vertex positions
for (int i = 0; i < internal_vts.size(); ++i) {
    OpenMesh::VertexHandle vh = internal_vts[i];
    halfedge_mesh->set_point(vh, { x_coords[i], y_coords[i], z_coords[i] });
}

    auto geometry = openmesh_to_operand(halfedge_mesh.get());

    // Set the output of the nodes
    params.set_output("Output", std::move(*geometry));
    return true;
}

NODE_DECLARATION_UI(min_surf);
NODE_DEF_CLOSE_SCOPE
