/******************************************************************************
 *
 * @file ungar/test/autodiff/function.test.cpp
 * @author Flavio De Vincenti (flavio.devincenti@inf.ethz.ch)
 *
 * @section LICENSE
 * -----------------------------------------------------------------------
 *
 * Copyright 2023 Flavio De Vincenti
 *
 * -----------------------------------------------------------------------
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the
 * License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS
 * IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
 *
 ******************************************************************************/

#include <gtest/gtest.h>

#include "ungar/autodiff/function.hpp"

namespace {

TEST(FunctionTest, ExponentialMap) {
    using namespace Ungar;
    using namespace Ungar::Autodiff;

    const index_t variableSize  = 3;
    const index_t parameterSize = 0;

    auto exp = []<typename _Scalar>(const VectorX<_Scalar>& x, VectorX<_Scalar>& y) -> void {
        y = Utils::ApproximateExponentialMap(RefToConstVector3<_Scalar>{x}).coeffs();
        /// @note CppADCodeGen does not support the following line.
        // Utils::ExponentialMap(RefToConstVector3<_Scalar>{independentVariable}).coeffs();
    };
    Function::Blueprint blueprint{
        exp, variableSize, parameterSize, "exponential_map_test", EnabledDerivatives::JACOBIAN};
    Function function = FunctionFactory::Make(blueprint, true);

    VectorXr x = Vector3r::Zero();
    ASSERT_TRUE(function.TestFunction(
        x, [&](const Vector3r& x) { return Utils::ExponentialMap(x).coeffs(); }));
    ASSERT_TRUE(function.TestJacobian(x));

    for (const auto i : enumerate(1024)) {
        x = Vector3r::Random();
        ASSERT_TRUE(function.TestFunction(
            x, [&](const Vector3r& x) { return Utils::ExponentialMap(x).coeffs(); }));
    }
}

TEST(FunctionTest, Jacobian) {
    using namespace Ungar;
    using namespace Ungar::Autodiff;

    constexpr auto VARIABLE_SIZE  = 4;
    constexpr auto PARAMETER_SIZE = 1;
    const index_t variableSize    = VARIABLE_SIZE;
    const index_t parameterSize   = PARAMETER_SIZE;

    auto f = []<typename _Scalar>(const VectorX<_Scalar>& xp, VectorX<_Scalar>& y) -> void {
        const auto [x, p] = Utils::Decompose<VARIABLE_SIZE, PARAMETER_SIZE>(xp);
        y                 = VectorX<_Scalar>{{p * x.squaredNorm(), 2.0 * pow(x[0_idx], 2)}};
    };
    Function::Blueprint blueprint{
        f, variableSize, parameterSize, "jacobian_test", EnabledDerivatives::JACOBIAN};
    Function function = FunctionFactory::Make(blueprint, true);

    const VectorXr x  = VectorXr::Random(variableSize);
    const VectorXr p  = VectorXr::Random(parameterSize);
    const VectorXr xp = Utils::Compose(x, p).ToDynamic();

    const VectorXr yGroundTruth = VectorXr{{p[0_idx] * x.squaredNorm(), 2.0 * pow(x[0_idx], 2)}};
    const MatrixXr jacobianGroundTruth = MatrixXr{{2.0 * p[0_idx] * x[0_idx],
                                                   2.0 * p[0_idx] * x[1_idx],
                                                   2.0 * p[0_idx] * x[2_idx],
                                                   2.0 * p[0_idx] * x[3_idx]},
                                                  {4.0 * x[0_idx], 0.0, 0.0, 0.0}};
    const MatrixXr hessianGroundTruth0 = 2.0 * p[0_idx] * MatrixXr::Identity(4_idx, 4_idx);
    const MatrixXr hessianGroundTruth1 =
        4.0 * VectorXr::Unit(4_idx, 0_idx) * VectorXr::Unit(4_idx, 0_idx).transpose();

    UNGAR_LOG(trace, "Testing function evaluation...");
    ASSERT_TRUE(function(xp).isApprox(yGroundTruth));
    UNGAR_LOG(trace, "Testing function Jacobian...");
    ASSERT_TRUE(function.Jacobian(xp).isApprox(jacobianGroundTruth));

    ASSERT_TRUE(function.Jacobian(xp).isApprox(jacobianGroundTruth));

    for (auto func =
             [&](const VectorXr& xp) {
                 VectorXr y;
                 f.template operator()<real_t>(xp, y);
                 return y;
             };
         const auto i : enumerate(1024)) {
        VectorXr xp = VectorXr::Random(variableSize + parameterSize);
        EXPECT_TRUE(function.TestFunction(xp, func));
        EXPECT_TRUE(function.TestJacobian(xp));
    }
}

TEST(FunctionTest, Hessian) {
    using namespace Ungar;
    using namespace Ungar::Autodiff;

    constexpr auto VARIABLE_SIZE  = 4;
    constexpr auto PARAMETER_SIZE = 1;
    const index_t variableSize    = VARIABLE_SIZE;
    const index_t parameterSize   = PARAMETER_SIZE;

    auto f = []<typename _Scalar>(const VectorX<_Scalar>& xp, VectorX<_Scalar>& y) -> void {
        const auto [x, p] = Utils::Decompose<VARIABLE_SIZE, PARAMETER_SIZE>(xp);
        y                 = VectorX<_Scalar>{{p * x.squaredNorm()}};
    };
    Function::Blueprint blueprint{
        f, variableSize, parameterSize, "hessian_test", EnabledDerivatives::HESSIAN};
    Function function = FunctionFactory::Make(blueprint, true);

    const VectorXr x  = VectorXr::Random(variableSize);
    const VectorXr p  = VectorXr::Random(parameterSize);
    const VectorXr xp = Utils::Compose(x, p).ToDynamic();

    const MatrixXr hessianGroundTruth = 2.0 * p[0_idx] * MatrixXr::Identity(4_idx, 4_idx);

    UNGAR_LOG(trace, "Testing function Hessian...");
    ASSERT_TRUE(function.Hessian(xp).isApprox(hessianGroundTruth));

    ASSERT_TRUE(function.Hessian(xp).isApprox(hessianGroundTruth));

    for (const auto i : enumerate(1024)) {
        VectorXr xp = VectorXr::Random(variableSize + parameterSize);
        EXPECT_TRUE(function.TestHessian(xp));
    }
}

}  // namespace

int main() {
    ::testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
