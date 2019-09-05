//
// Created by ljn on 19-8-16.
//

#ifndef MPC_PATH_OPTIMIZER__FGEVALFRENET_HPP_
#define MPC_PATH_OPTIMIZER__FGEVALFRENET_HPP_
#include <vector>

namespace MpcSmoother {
using CppAD::AD;

class FgEvalFrenet {
public:
    FgEvalFrenet(const std::vector<double> &seg_x_list,
                 const std::vector<double> &seg_y_list,
                 const std::vector<double> &seg_angle_list,
                 const std::vector<double> &seg_k_list,
                 const std::vector<double> &seg_s_list,
                 const int &N,
                 const std::vector<double> &cost_func) :
        N(N),
        seg_s_list_(seg_s_list),
        seg_x_list_(seg_x_list),
        seg_y_list_(seg_y_list),
        seg_angle_list_(seg_angle_list),
        seg_k_list_(seg_k_list),
        cost_func_curvature_weight_(cost_func[0]),
        cost_func_curvature_rate_weight_(cost_func[1]) {}

public:
    size_t N;
    const std::vector<double> &seg_s_list_;
    const std::vector<double> &seg_x_list_;
    const std::vector<double> &seg_y_list_;
    const std::vector<double> &seg_k_list_;
    const std::vector<double> &seg_angle_list_;

    double cost_func_curvature_weight_;
    double cost_func_curvature_rate_weight_;

    typedef CPPAD_TESTVECTOR(AD<double>) ADvector;

    void operator()(ADvector &fg, const ADvector &vars) {

        const size_t pq_range_begin = 0;
        const size_t heading_range_begin = pq_range_begin + N;
        const size_t curvature_range_begin = heading_range_begin + 1;

        const size_t cons_heading_range_begin = 1;
        const size_t cons_curvature_range_begin = cons_heading_range_begin + 1;

        for (int t = 0; t < N - 2; t++) {
            fg[0] += cost_func_curvature_weight_ * pow(vars[curvature_range_begin + t], 2);
        }
        for (int t = 0; t < N - 3; t++) {
            fg[0] += cost_func_curvature_rate_weight_
                * pow(vars[curvature_range_begin + t + 1] - vars[curvature_range_begin + t], 2);
        }

        // The rest of the constraints
        for (size_t i = 1; i <= N - 2; i++) {
            size_t i_before = i - 1;
            size_t i_after = i + 1;

            AD<double> curvature = vars[curvature_range_begin + i - 1];

            AD<double> pq_before = vars[pq_range_begin + i_before];
            AD<double> pq = vars[pq_range_begin + i];
            AD<double> pq_after = vars[pq_range_begin + i_after];

            AD<double> ref_x_before = seg_x_list_[i_before];
            AD<double> ref_y_before = seg_y_list_[i_before];
            AD<double> ref_angle_before = seg_angle_list_[i_before];
            AD<double> ref_x = seg_x_list_[i];
            AD<double> ref_y = seg_y_list_[i];
            AD<double> ref_angle = seg_angle_list_[i];
            AD<double> ref_x_after = seg_x_list_[i_after];
            AD<double> ref_y_after = seg_y_list_[i_after];
            AD<double> ref_angle_after = seg_angle_list_[i_after];

            AD<double> x_before = ref_x_before + pq_before * CppAD::cos(ref_angle_before + M_PI_2);
            AD<double> y_before = ref_y_before + pq_before * CppAD::sin(ref_angle_before + M_PI_2);
            AD<double> x = ref_x + pq * CppAD::cos(ref_angle + M_PI_2);
            AD<double> y = ref_y + pq * CppAD::sin(ref_angle + M_PI_2);
            AD<double> x_after = ref_x_after + pq_after * CppAD::cos(ref_angle_after + M_PI_2);
            AD<double> y_after = ref_y_after + pq_after * CppAD::sin(ref_angle_after + M_PI_2);

            AD<double> heading_by_position;
            if (i == N - 2) {
                heading_by_position = CppAD::atan2(y_after - y, x_after - x);
                AD<double> heading = vars[heading_range_begin];
                fg[cons_heading_range_begin] = heading - heading_by_position;
            }

            // Two methods to calculate curvature. The first method is more accurate while the second method runs faster.
//            AD<double> curvature_by_position;
//            if (seg_x_list_[i] - seg_x_list_[i_before] < 0) {
//                AD<double> heading = CppAD::atan2(-y_after + y, -x_after + x);
//                AD<double> heading_before = CppAD::atan2(-y + y_before, -x + x_before);
//                AD<double> ds = CppAD::fabs((x - x_before) / CppAD::cos(heading_before));
//                curvature_by_position = (heading - heading_before) / ds;
//            } else {
//                AD<double> heading = CppAD::atan2(y_after - y, x_after - x);
//                AD<double> heading_before = CppAD::atan2(y - y_before, x - x_before);
//                AD<double> ds = CppAD::fabs((x - x_before) / CppAD::cos(heading_before));
//                curvature_by_position = (heading - heading_before) / ds;
//            }

            AD<double> ref_ds_before = seg_s_list_[i] - seg_s_list_[i_before];
            AD<double> psi_before = (pq - pq_before) / ref_ds_before;
            AD<double> ps_before = ref_ds_before / CppAD::cos(psi_before) * (1 - pq_before * seg_k_list_[i_before]);
            AD<double> ref_ds_after = seg_s_list_[i_after] - seg_s_list_[i];
            AD<double> psi_after = (pq_after - pq) / ref_ds_after;
            AD<double> curvature_by_position = (psi_after - psi_before) / ps_before + seg_k_list_[i];

            fg[cons_curvature_range_begin + i - 1] = curvature - curvature_by_position;
        }
    }
};
}

#endif //MPC_PATH_OPTIMIZER__FGEVALFRENET_HPP_
