#include "pch.h"
#include <DirectXMath.h>

using namespace DirectX;

TEST(MathTest, VectorAddition) {
    XMVECTOR v1 = XMVectorSet(1.0f, 2.0f, 3.0f, 0.0f);
    XMVECTOR v2 = XMVectorSet(4.0f, 5.0f, 6.0f, 0.0f);
    XMVECTOR vResult = XMVectorAdd(v1, v2);

    XMFLOAT3 result;
    XMStoreFloat3(&result, vResult);

    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 7.0f);
    EXPECT_FLOAT_EQ(result.z, 9.0f);
}

TEST(MathTest, MatrixIdentity) {
    XMMATRIX m = XMMatrixIdentity();
    XMFLOAT4X4 f;
    XMStoreFloat4x4(&f, m);

    EXPECT_FLOAT_EQ(f._11, 1.0f);
    EXPECT_FLOAT_EQ(f._22, 1.0f);
    EXPECT_FLOAT_EQ(f._33, 1.0f);
    EXPECT_FLOAT_EQ(f._44, 1.0f);
    EXPECT_FLOAT_EQ(f._12, 0.0f);
}
