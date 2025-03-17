#include "seamless_clone.h"
#include <vector>
#include <algorithm>
#include <cmath>        // for std::clamp
#include "common/image.h"

SeamlessClone::SeamlessClone(
    std::shared_ptr<USTC_CG::Image> src,
    std::shared_ptr<USTC_CG::Image> tar,
    std::shared_ptr<USTC_CG::Image> mask,
    int offset_x, int offset_y)
    : src_img_(src), tar_img_(tar), mask_(mask),
      offset_x_(offset_x), offset_y_(offset_y)
{
    // 构建 \Omega 集合，并建立坐标->索引的映射
    buildSystem();
}

void SeamlessClone::buildSystem()
{
    omega_points_.clear();
    coord_to_index_.clear();

    int src_w = src_img_->width();
    int src_h = src_img_->height();
    int mask_w = mask_->width();
    int mask_h = mask_->height();

    int tar_w = tar_img_->width();
    int tar_h = tar_img_->height();

    for(int sy = 0; sy < mask_h; ++sy)
    {
        for(int sx = 0; sx < mask_w; ++sx)
        {
            // 如果 mask 标记此像素为 255，表示选中
            if(mask_->get_pixel(sx, sy)[0] > 0)
            {
                // 在目标图中的位置
                int tx = sx + offset_x_;
                int ty = sy + offset_y_;

                // 检查是否在目标图范围
                if (tx >= 0 && tx < tar_w && ty >= 0 && ty < tar_h)
                {
                    // 同时要确保在源图像范围内
                    if (sx >= 0 && sx < src_w && sy >= 0 && sy < src_h)
                    {
                        // 将 (tx, ty) 纳入 \Omega
                        omega_points_.emplace_back(tx, ty);
                        // 记录索引
                        coord_to_index_[{tx, ty}] = (int)omega_points_.size() - 1;
                    }
                }
            }
        }
    }

    // 每个通道都需要构造自己的稀疏矩阵
    for (int c = 0; c < 3; ++c)
    {
        channels_[c].needs_rebuild = true;
    }
}

std::shared_ptr<USTC_CG::Image> SeamlessClone::solve(bool realtime)
{
    // 拷贝一个目标图，最终将融合结果写到此图
    auto result = std::make_shared<USTC_CG::Image>(*tar_img_);
    int num_pixels = (int)omega_points_.size();
    if (num_pixels == 0)
    {
        // 没有可融合区域，直接返回
        return result;
    }

    for (int c = 0; c < 3; ++c)
    {
        // 如果矩阵 A 需要重建，则重新 build & 分解
        if (channels_[c].needs_rebuild)
        {
            buildAMatrix(c);
            channels_[c].solver.compute(channels_[c].A);
            channels_[c].needs_rebuild = false;
        }

        // 构造右端向量 B
        Eigen::VectorXd B(num_pixels);
        buildBVector(c, B);

        // 解方程组
        Eigen::VectorXd solution = channels_[c].solver.solve(B);

        // 将解写回目标图（做截断到 [0,255]）
        for (int i = 0; i < num_pixels; ++i)
        {
            auto [x, y] = omega_points_[i];
            auto pixel = result->get_pixel(x, y);

            double val = std::clamp(solution[i], 0.0, 255.0);
            pixel[c] = static_cast<unsigned char>(val);

            result->set_pixel(x, y, pixel);
        }
    }

    return result;
}

// 构建系数矩阵 A
void SeamlessClone::buildAMatrix(int c)
{
    int num_pixels = (int)omega_points_.size();
    channels_[c].A.resize(num_pixels, num_pixels);   // 大小为 num_pixels * num_pixels
    channels_[c].A.setZero();

    // 用 Triplet 存储稀疏矩阵的非零项
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.reserve(num_pixels * 5); // 粗略估计，每行最多 5 个非零

    // 目标图的宽高
    int tar_w = tar_img_->width();
    int tar_h = tar_img_->height();

    // 对 \Omega 内的每个像素 i，找它的 4 个邻居
    // 对角元通常为“有效邻居数”，邻居列为 -1
    for (int i = 0; i < num_pixels; ++i)
    {
        auto [x, y] = omega_points_[i];

        // 收集上下左右
        std::vector<std::pair<int,int>> neighbors = {
            {x, y-1}, {x, y+1}, {x-1, y}, {x+1, y}
        };

        // 统计这个像素有多少个“在目标图范围内”的邻居
        int nb_count = 0;
        // 记录在 \Omega 内的邻居索引
        std::vector<int> inside_omega_nb;

        for (auto& nb : neighbors)
        {
            auto [nx, ny] = nb;
            if (nx >= 0 && nx < tar_w && ny >= 0 && ny < tar_h)
            {
                nb_count++;
                // 如果邻居也在 \Omega 内
                if(coord_to_index_.count({nx, ny}))
                {
                    inside_omega_nb.push_back(coord_to_index_.at({nx, ny}));
                }
            }
        }

        // 对角元设置为有效邻居数量
        triplets.emplace_back(i, i, (double)nb_count);

        // 对于在 \Omega 内的邻居像素 j，系数为 -1
        for (int j : inside_omega_nb)
        {
            triplets.emplace_back(i, j, -1.0);
        }
    }

    // 用三元组初始化 A
    channels_[c].A.setFromTriplets(triplets.begin(), triplets.end());
}

// 构建右端向量 B
void SeamlessClone::buildBVector(int c, Eigen::VectorXd& B)
{
    int num_pixels = (int)omega_points_.size();
    B.setZero();

    // 源图、目标图宽高
    int src_w = src_img_->width();
    int src_h = src_img_->height();
    int tar_w = tar_img_->width();
    int tar_h = tar_img_->height();

    // 对每个像素 i，构造 B[i]
    for (int i = 0; i < num_pixels; ++i)
    {
        auto [tx, ty] = omega_points_[i];   // 目标图坐标
        int sx = tx - offset_x_;            // 对应源图坐标
        int sy = ty - offset_y_;

        // 源图像像素值 g_i，越界则记为 0
        double g_i = 0.0;
        if (sx >= 0 && sx < src_w && sy >= 0 && sy < src_h)
        {
            g_i = src_img_->get_pixel(sx, sy)[c];
        }

        // 邻居(上下左右)
        std::vector<std::pair<int,int>> neighbors = {
            {tx, ty-1}, {tx, ty+1}, {tx-1, ty}, {tx+1, ty}
        };
        int nb_count = 0;

        double sum_g_j = 0.0;     // 源图像邻居颜色之和
        double sum_f_star = 0.0;  // 目标图边界值之和

        for (auto& nb : neighbors)
        {
            auto [nx, ny] = nb;
            if (nx >= 0 && nx < tar_w && ny >= 0 && ny < tar_h)
            {
                nb_count++; // 有效邻居计数

                // 计算该邻居在源图像中的坐标
                int s_nx = nx - offset_x_;
                int s_ny = ny - offset_y_;
                double g_j = 0.0;

                // 只要源图像坐标有效，就累加其值
                if (s_nx >= 0 && s_nx < src_w && s_ny >= 0 && s_ny < src_h)
                {
                    g_j = src_img_->get_pixel(s_nx, s_ny)[c];
                }
                sum_g_j += g_j;

                // 检查邻居是否在Ω内
                if (!coord_to_index_.count({nx, ny}))
                {
                    // 不在Ω内，累加目标图像边界值
                    sum_f_star += tar_img_->get_pixel(nx, ny)[c];
                }
            }
        }

        // 按照离散拉普拉斯公式: nb_count*g_i - sum_g_j
        // 并加入外邻居的已知值 sum_f_star
        B[i] = nb_count * g_i - sum_g_j + sum_f_star;
    }
}
