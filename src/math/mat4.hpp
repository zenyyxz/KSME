/*
 * 4x4 HOMOGENEOUS TRANSFORMATION PIPELINE
 * Implements linear algebra for 3D-to-2D projection. 
 * Standard column-major logic for world-view-projection transforms.
 */

#pragma once
#include "vec3.hpp"
#include <cmath>
#include <cstring>

namespace math {

struct Mat4 {
    float m[4][4];

    Mat4() {
        std::memset(m, 0, sizeof(m));
    }

    static Mat4 identity() {
        Mat4 mat;
        mat.m[0][0] = 1; mat.m[1][1] = 1; mat.m[2][2] = 1; mat.m[3][3] = 1;
        return mat;
    }

    Vec4 operator*(const Vec4& v) const {
        return {
            v.x * m[0][0] + v.y * m[0][1] + v.z * m[0][2] + v.w * m[0][3],
            v.x * m[1][0] + v.y * m[1][1] + v.z * m[1][2] + v.w * m[1][3],
            v.x * m[2][0] + v.y * m[2][1] + v.z * m[2][2] + v.w * m[2][3],
            v.x * m[3][0] + v.y * m[3][1] + v.z * m[3][2] + v.w * m[3][3]
        };
    }

    Mat4 operator*(const Mat4& other) const {
        Mat4 res;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                for (int k = 0; k < 4; ++k)
                    res.m[i][j] += m[i][k] * other.m[k][j];
        return res;
    }

    static Mat4 translation(float x, float y, float z) {
        Mat4 mat = identity();
        mat.m[0][3] = x; mat.m[1][3] = y; mat.m[2][3] = z;
        return mat;
    }

    static Mat4 scale(float x, float y, float z) {
        Mat4 mat = identity();
        mat.m[0][0] = x; mat.m[1][1] = y; mat.m[2][2] = z;
        return mat;
    }

    static Mat4 rotationX(float angle) {
        Mat4 mat = identity();
        float c = std::cos(angle);
        float s = std::sin(angle);
        mat.m[1][1] = c; mat.m[1][2] = -s;
        mat.m[2][1] = s; mat.m[2][2] = c;
        return mat;
    }

    static Mat4 rotationY(float angle) {
        Mat4 mat = identity();
        float c = std::cos(angle);
        float s = std::sin(angle);
        mat.m[0][0] = c; mat.m[0][2] = s;
        mat.m[2][0] = -s; mat.m[2][2] = c;
        return mat;
    }

    static Mat4 rotationZ(float angle) {
        Mat4 mat = identity();
        float c = std::cos(angle);
        float s = std::sin(angle);
        mat.m[0][0] = c; mat.m[0][1] = -s;
        mat.m[1][0] = s; mat.m[1][1] = c;
        return mat;
    }

    // Generates a perspective projection matrix to simulate depth and FOV.
    static Mat4 perspective(float fov, float aspect, float near, float far) {
        Mat4 mat;
        float f = 1.0f / std::tan(fov / 2.0f);
        mat.m[0][0] = f / aspect;
        mat.m[1][1] = f;
        mat.m[2][2] = (far + near) / (near - far);
        mat.m[2][3] = (2.0f * far * near) / (near - far);
        mat.m[3][2] = -1.0f;
        return mat;
    }

    // Generates a View Matrix based on eye, target, and world-up vectors.
    static Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) {
        Vec3 f = (target - eye).normalize();
        Vec3 s = f.cross(up).normalize();
        Vec3 u = s.cross(f);

        Mat4 mat = identity();
        mat.m[0][0] = s.x; mat.m[0][1] = s.y; mat.m[0][2] = s.z;
        mat.m[1][0] = u.x; mat.m[1][1] = u.y; mat.m[1][2] = u.z;
        mat.m[2][0] = -f.x; mat.m[2][1] = -f.y; mat.m[2][2] = -f.z;
        mat.m[0][3] = -s.dot(eye);
        mat.m[1][3] = -u.dot(eye);
        mat.m[2][3] = f.dot(eye);
        return mat;
    }
};

} // namespace math
