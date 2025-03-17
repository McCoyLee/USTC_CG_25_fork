#pragma once
#include <memory>
#include <Eigen/Sparse>
#include <common/image.h>

class Image;

class SeamlessClone
{
    public:
        SeamlessClone(
            std::shared_ptr<USTC_CG::Image> src,
            std::shared_ptr<USTC_CG::Image> tar,
            std::shared_ptr<USTC_CG::Image> mask,
            int offset_x, int offset_y);
    std::shared_ptr<USTC_CG::Image> solve(bool realtime = false);    

    private:
            std::shared_ptr<USTC_CG::Image> src_img_;
            std::shared_ptr<USTC_CG::Image> tar_img_;
            std::shared_ptr<USTC_CG::Image> mask_;
            int offset_x_, offset_y_;
    
    struct ChannelData {
        Eigen::SparseMatrix<double> A;
        Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver;
        bool needs_rebuild = true;
    };

    std::array<ChannelData, 3> channels_;
    std::vector<std::pair<int, int>> omega_points_;
    std::map<std::pair<int, int>, int> coord_to_index_;

    void buildSystem();
    void buildAMatrix(int c);
    void buildBVector(int c, Eigen::VectorXd& B);
};