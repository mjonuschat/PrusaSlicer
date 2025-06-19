/*
 *  THIS FILE IS A MODIFICATION OF THE ORIGINAL ImGuizmo.cpp file from LIBRARY:
 *  https://github.com/CedricGuillemet/ImGuizmo
 *  WHOSE LICENSE IS AS FOLLOWS:
 *
 *  v1.91.3 WIP
 *
 *  The MIT License(MIT)
 *
 *  Copyright(c) 2021 Cedric Guillemet
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files(the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions :
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#include "Slic3r/App/Imgui/NavigationCube.hpp"
#include "Slic3r/Math.hpp"

#include <array>
#include <string>

#include <math.h>

namespace Slic3r::App::Imgui::NavCube {

///
/// TODO: It would be nice to replace all the following math types with PrusaSlicer's ones
///

    static void fpu_matrix_x_matrix(const float* a, const float* b, float* r)
    {
        r[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
        r[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
        r[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
        r[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];

        r[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
        r[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
        r[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
        r[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

        r[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
        r[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
        r[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
        r[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];

        r[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
        r[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
        r[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
        r[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
    }

    static void frustum(float left, float right, float bottom, float top, float znear, float zfar, float* m16)
    {
        float temp = 2.0f * znear;
        float temp2 = right - left;
        float temp3 = top - bottom;
        float temp4 = zfar - znear;
        m16[0] = temp / temp2;
        m16[1] = 0.0f;
        m16[2] = 0.0f;
        m16[3] = 0.0f;
        m16[4] = 0.0f;
        m16[5] = temp / temp3;
        m16[6] = 0.0f;
        m16[7] = 0.0f;
        m16[8] = (right + left) / temp2;
        m16[9] = (top + bottom) / temp3;
        m16[10] = (-zfar - znear) / temp4;
        m16[11] = -1.0f;
        m16[12] = 0.0f;
        m16[13] = 0.0f;
        m16[14] = (-temp * zfar) / temp4;
        m16[15] = 0.0f;
    }

    static void perspective(float fovy_in_degrees, float aspect_ratio, float z_near, float z_far, float* m16)
    {
        float ymax = z_near * tanf(deg2rad(fovy_in_degrees));
        float xmax = ymax * aspect_ratio;
        frustum(-xmax, xmax, -ymax, ymax, z_near, z_far, m16);
    }

    static void orthographic(float l, float r, float b, float t, float zn, float zf, float* m16)
    {
        m16[0]  = 2.0f / (r - l);
        m16[1]  = 0.0f;
        m16[2]  = 0.0f;
        m16[3]  = 0.0f;
        m16[4]  = 0.0f;
        m16[5]  = 2.0f / (t - b);
        m16[6]  = 0.0f;
        m16[7]  = 0.0f;
        m16[8]  = 0.0f;
        m16[9]  = 0.0f;
        m16[10] = 1.0f / (zf - zn);
        m16[11] = 0.0f;
        m16[12] = (l + r) / (l - r);
        m16[13] = (t + b) / (b - t);
        m16[14] = zn / (zn - zf);
        m16[15] = 1.0f;
    }

    static void cross(const float* a, const float* b, float* r)
    {
        r[0] = a[1] * b[2] - a[2] * b[1];
        r[1] = a[2] * b[0] - a[0] * b[2];
        r[2] = a[0] * b[1] - a[1] * b[0];
    }

    static float dot(const float* a, const float* b)
    {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }

    static void normalize(const float* a, float* r)
    {
        float il = 1.0f / (sqrtf(dot(a, a)) + FLT_EPSILON);
        r[0] = a[0] * il;
        r[1] = a[1] * il;
        r[2] = a[2] * il;
    }

    static void look_at(const float* eye, const float* at, const float* up, float* m16)
    {
        float X[3], Y[3], Z[3], tmp[3];

        tmp[0] = eye[0] - at[0];
        tmp[1] = eye[1] - at[1];
        tmp[2] = eye[2] - at[2];
        normalize(tmp, Z);
        normalize(up, Y);
        cross(Y, Z, tmp);
        normalize(tmp, X);
        cross(Z, X, tmp);
        normalize(tmp, Y);

        m16[0] = X[0];
        m16[1] = Y[0];
        m16[2] = Z[0];
        m16[3] = 0.0f;
        m16[4] = X[1];
        m16[5] = Y[1];
        m16[6] = Z[1];
        m16[7] = 0.0f;
        m16[8] = X[2];
        m16[9] = Y[2];
        m16[10] = Z[2];
        m16[11] = 0.0f;
        m16[12] = -dot(X, eye);
        m16[13] = -dot(Y, eye);
        m16[14] = -dot(Z, eye);
        m16[15] = 1.0f;
    }

    struct matrix_t;
    struct vec_t
    {
    public:
        float x, y, z, w;

        void lerp(const vec_t& v, float t) {
            x += (v.x - x) * t;
            y += (v.y - y) * t;
            z += (v.z - z) * t;
            w += (v.w - w) * t;
        }

        void set(float v) {
            x = y = z = w = v;
        }
        
        void set(float _x, float _y, float _z = 0.0f, float _w = 0.0f) {
            x = _x;
            y = _y;
            z = _z;
            w = _w;
        }

        vec_t& operator -= (const vec_t& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
        vec_t& operator += (const vec_t& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
        vec_t& operator *= (const vec_t& v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
        vec_t& operator *= (float v)        { x *= v;   y *= v;   z *= v;   w *= v;   return *this; }

        vec_t operator * (float f) const;
        vec_t operator - () const;
        vec_t operator - (const vec_t& v) const;
        vec_t operator + (const vec_t& v) const;
        vec_t operator * (const vec_t& v) const;

        const vec_t& operator + () const { return (*this); }

        float length() const { return sqrtf(x * x + y * y + z * z); };
        float length_sq() const { return (x * x + y * y + z * z); };
        vec_t normalize() { (*this) *= (1.0f / ( length() > FLT_EPSILON ? length() : FLT_EPSILON ) ); return (*this); }
        vec_t normalize(const vec_t& v) { set(v.x, v.y, v.z, v.w); normalize(); return (*this); }
        vec_t abs() const;

        void cross(const vec_t& v) {
            vec_t res;
            res.x = y * v.z - z * v.y;
            res.y = z * v.x - x * v.z;
            res.z = x * v.y - y * v.x;

            x = res.x;
            y = res.y;
            z = res.z;
            w = 0.0f;
        }

        void cross(const vec_t& v1, const vec_t& v2) {
            x = v1.y * v2.z - v1.z * v2.y;
            y = v1.z * v2.x - v1.x * v2.z;
            z = v1.x * v2.y - v1.y * v2.x;
            w = 0.0f;
        }

        float dot(const vec_t& v) const {
            return (x * v.x) + (y * v.y) + (z * v.z) + (w * v.w);
        }

        float dot3(const vec_t& v) const {
            return (x * v.x) + (y * v.y) + (z * v.z);
        }

        void transform(const matrix_t& matrix);
        void transform(const vec_t& s, const matrix_t& matrix);

        void transform_vector(const matrix_t& matrix);
        void transform_point(const matrix_t& matrix);
        void transform_vector(const vec_t& v, const matrix_t& matrix) { (*this) = v; transform_vector(matrix); }
        void transform_point(const vec_t& v, const matrix_t& matrix) { (*this) = v; transform_point(matrix); }

        float& operator [] (size_t index) { return ((float*)&x)[index]; }
        const float& operator [] (size_t index) const { return ((float*)&x)[index]; }
        bool operator!=(const vec_t& other) const { return memcmp(this, &other, sizeof(vec_t)) != 0; }
    };

    vec_t make_vect(float _x, float _y, float _z = 0.0f, float _w = 0.0f) { vec_t res; res.x = _x; res.y = _y; res.z = _z; res.w = _w; return res; }
    vec_t make_vect(ImVec2 v) { vec_t res; res.x = v.x; res.y = v.y; res.z = 0.0f; res.w = 0.0f; return res; }
    vec_t vec_t::operator * (float f) const { return make_vect(x * f, y * f, z * f, w * f); }
    vec_t vec_t::operator - () const { return make_vect(-x, -y, -z, -w); }
    vec_t vec_t::operator - (const vec_t& v) const { return make_vect(x - v.x, y - v.y, z - v.z, w - v.w); }
    vec_t vec_t::operator + (const vec_t& v) const { return make_vect(x + v.x, y + v.y, z + v.z, w + v.w); }
    vec_t vec_t::operator * (const vec_t& v) const { return make_vect(x * v.x, y * v.y, z * v.z, w * v.w); }
    vec_t vec_t::abs() const { return make_vect(fabsf(x), fabsf(y), fabsf(z)); }

    vec_t normalized(const vec_t& v) { vec_t res; res = v; res.normalize(); return res; }
    vec_t cross(const vec_t& v1, const vec_t& v2)
    {
        vec_t res;
        res.x = v1.y * v2.z - v1.z * v2.y;
        res.y = v1.z * v2.x - v1.x * v2.z;
        res.z = v1.x * v2.y - v1.y * v2.x;
        res.w = 0.0f;
        return res;
    }

    float dot(const vec_t& v1, const vec_t& v2)
    {
        return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
    }

    static vec_t build_plan(const vec_t& p_point, const vec_t& p_normal)
    {
        vec_t normal, res;
        normal.normalize(p_normal);
        res.w = normal.dot(p_point);
        res.x = normal.x;
        res.y = normal.y;
        res.z = normal.z;
        return res;
    }

    struct matrix_t
    {
    public:
        union
        {
            float m[4][4];
            float m16[16];
            struct
            {
                vec_t right, up, dir, position;
            } v;
            vec_t component[4];
        };

        operator float* () { return m16; }
        operator const float* () const { return m16; }
        void translation(float _x, float _y, float _z) { translation(make_vect(_x, _y, _z)); }

        void translation(const vec_t& vt) {
            v.right.set(1.0f, 0.0f, 0.0f, 0.0f);
            v.up.set(0.0f, 1.0f, 0.0f, 0.0f);
            v.dir.set(0.0f, 0.0f, 1.0f, 0.0f);
            v.position.set(vt.x, vt.y, vt.z, 1.0f);
        }

        void scale(float _x, float _y, float _z) {
            v.right.set(_x, 0.0f, 0.0f, 0.0f);
            v.up.set(0.0f, _y, 0.0f, 0.0f);
            v.dir.set(0.0f, 0.0f, _z, 0.0f);
            v.position.set(0.0f, 0.0f, 0.0f, 1.0f);
        }

        void scale(const vec_t& s) {
            scale(s.x, s.y, s.z);
        }

        matrix_t& operator *= (const matrix_t& mat) {
            matrix_t tmpMat;
            tmpMat = *this;
            tmpMat.multiply(mat);
            *this = tmpMat;
            return *this;
        }

        matrix_t operator * (const matrix_t& mat) const {
            matrix_t matT;
            matT.multiply(*this, mat);
            return matT;
        }

        void multiply(const matrix_t& matrix) {
            matrix_t tmp;
            tmp = *this;
            fpu_matrix_x_matrix((float*)&tmp, (float*)&matrix, (float*)this);
        }

        void multiply(const matrix_t& m1, const matrix_t& m2) {
            fpu_matrix_x_matrix((float*)&m1, (float*)&m2, (float*)this);
        }

        float determinant() const {
            return m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] + m[0][2] * m[1][0] * m[2][1] -
                   m[0][2] * m[1][1] * m[2][0] - m[0][1] * m[1][0] * m[2][2] - m[0][0] * m[1][2] * m[2][1];
        }

        float inverse(const matrix_t& srcMatrix, bool affine = false) {
            float det = 0.0f;

            if (affine) {
                det = determinant();
                float s = 1.0f / det;
                m[0][0] = (srcMatrix.m[1][1] * srcMatrix.m[2][2] - srcMatrix.m[1][2] * srcMatrix.m[2][1]) * s;
                m[0][1] = (srcMatrix.m[2][1] * srcMatrix.m[0][2] - srcMatrix.m[2][2] * srcMatrix.m[0][1]) * s;
                m[0][2] = (srcMatrix.m[0][1] * srcMatrix.m[1][2] - srcMatrix.m[0][2] * srcMatrix.m[1][1]) * s;
                m[1][0] = (srcMatrix.m[1][2] * srcMatrix.m[2][0] - srcMatrix.m[1][0] * srcMatrix.m[2][2]) * s;
                m[1][1] = (srcMatrix.m[2][2] * srcMatrix.m[0][0] - srcMatrix.m[2][0] * srcMatrix.m[0][2]) * s;
                m[1][2] = (srcMatrix.m[0][2] * srcMatrix.m[1][0] - srcMatrix.m[0][0] * srcMatrix.m[1][2]) * s;
                m[2][0] = (srcMatrix.m[1][0] * srcMatrix.m[2][1] - srcMatrix.m[1][1] * srcMatrix.m[2][0]) * s;
                m[2][1] = (srcMatrix.m[2][0] * srcMatrix.m[0][1] - srcMatrix.m[2][1] * srcMatrix.m[0][0]) * s;
                m[2][2] = (srcMatrix.m[0][0] * srcMatrix.m[1][1] - srcMatrix.m[0][1] * srcMatrix.m[1][0]) * s;
                m[3][0] = -(m[0][0] * srcMatrix.m[3][0] + m[1][0] * srcMatrix.m[3][1] + m[2][0] * srcMatrix.m[3][2]);
                m[3][1] = -(m[0][1] * srcMatrix.m[3][0] + m[1][1] * srcMatrix.m[3][1] + m[2][1] * srcMatrix.m[3][2]);
                m[3][2] = -(m[0][2] * srcMatrix.m[3][0] + m[1][2] * srcMatrix.m[3][1] + m[2][2] * srcMatrix.m[3][2]);
            }
            else {
                // transpose matrix
                float src[16];
                for (int i = 0; i < 4; ++i) {
                    src[i] = srcMatrix.m16[i * 4];
                    src[i + 4] = srcMatrix.m16[i * 4 + 1];
                    src[i + 8] = srcMatrix.m16[i * 4 + 2];
                    src[i + 12] = srcMatrix.m16[i * 4 + 3];
                }

                // calculate pairs for first 8 elements (cofactors)
                float tmp[12]; // temp array for pairs
                tmp[0] = src[10] * src[15];
                tmp[1] = src[11] * src[14];
                tmp[2] = src[9] * src[15];
                tmp[3] = src[11] * src[13];
                tmp[4] = src[9] * src[14];
                tmp[5] = src[10] * src[13];
                tmp[6] = src[8] * src[15];
                tmp[7] = src[11] * src[12];
                tmp[8] = src[8] * src[14];
                tmp[9] = src[10] * src[12];
                tmp[10] = src[8] * src[13];
                tmp[11] = src[9] * src[12];

                // calculate first 8 elements (cofactors)
                m16[0] = (tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7]) - (tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7]);
                m16[1] = (tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7]) - (tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7]);
                m16[2] = (tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7]) - (tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7]);
                m16[3] = (tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6]) - (tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6]);
                m16[4] = (tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3]) - (tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3]);
                m16[5] = (tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3]) - (tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3]);
                m16[6] = (tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3]) - (tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3]);
                m16[7] = (tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2]) - (tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2]);

                // calculate pairs for second 8 elements (cofactors)
                tmp[0] = src[2] * src[7];
                tmp[1] = src[3] * src[6];
                tmp[2] = src[1] * src[7];
                tmp[3] = src[3] * src[5];
                tmp[4] = src[1] * src[6];
                tmp[5] = src[2] * src[5];
                tmp[6] = src[0] * src[7];
                tmp[7] = src[3] * src[4];
                tmp[8] = src[0] * src[6];
                tmp[9] = src[2] * src[4];
                tmp[10] = src[0] * src[5];
                tmp[11] = src[1] * src[4];

                // calculate second 8 elements (cofactors)
                m16[8] = (tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15]) - (tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15]);
                m16[9] = (tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15]) - (tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15]);
                m16[10] = (tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15]) - (tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15]);
                m16[11] = (tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14]) - (tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14]);
                m16[12] = (tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9]) - (tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10]);
                m16[13] = (tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10]) - (tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8]);
                m16[14] = (tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8]) - (tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9]);
                m16[15] = (tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9]) - (tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8]);

                // calculate determinant
                det = src[0] * m16[0] + src[1] * m16[1] + src[2] * m16[2] + src[3] * m16[3];

                // calculate matrix inverse
                float invdet = 1.0f / det;
                for (int j = 0; j < 16; ++j) {
                    m16[j] *= invdet;
                }
            }

            return det;
        }

        void set_to_identity() {
            v.right.set(1.0f, 0.0f, 0.0f, 0.0f);
            v.up.set(0.0f, 1.0f, 0.0f, 0.0f);
            v.dir.set(0.0f, 0.0f, 1.0f, 0.0f);
            v.position.set(0.0f, 0.0f, 0.0f, 1.0f);
        }

        void transpose() {
            matrix_t tmpm;
            for (int l = 0; l < 4; l++) {
                for (int c = 0; c < 4; c++) {
                    tmpm.m[l][c] = m[c][l];
                }
            }
            (*this) = tmpm;
        }

        void rotation_axis(const vec_t& axis, float angle) {
            float length2 = axis.length_sq();
            if (length2 < FLT_EPSILON) {
                set_to_identity();
                return;
            }

            vec_t n = axis * (1.0f / sqrtf(length2));
            float s = sinf(angle);
            float c = cosf(angle);
            float k = 1.0f - c;

            float xx = n.x * n.x * k + c;
            float yy = n.y * n.y * k + c;
            float zz = n.z * n.z * k + c;
            float xy = n.x * n.y * k;
            float yz = n.y * n.z * k;
            float zx = n.z * n.x * k;
            float xs = n.x * s;
            float ys = n.y * s;
            float zs = n.z * s;

            m[0][0] = xx;
            m[0][1] = xy + zs;
            m[0][2] = zx - ys;
            m[0][3] = 0.0f;
            m[1][0] = xy - zs;
            m[1][1] = yy;
            m[1][2] = yz + xs;
            m[1][3] = 0.0f;
            m[2][0] = zx + ys;
            m[2][1] = yz - xs;
            m[2][2] = zz;
            m[2][3] = 0.0f;
            m[3][0] = 0.0f;
            m[3][1] = 0.0f;
            m[3][2] = 0.0f;
            m[3][3] = 1.0f;
        }

        void orthonormalize() {
            v.right.normalize();
            v.up.normalize();
            v.dir.normalize();
        }
    };

    void vec_t::transform(const matrix_t& matrix)
    {
        vec_t out;

        out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0] + w * matrix.m[3][0];
        out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1] + w * matrix.m[3][1];
        out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2] + w * matrix.m[3][2];
        out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3] + w * matrix.m[3][3];

        x = out.x;
        y = out.y;
        z = out.z;
        w = out.w;
    }

    void vec_t::transform(const vec_t& s, const matrix_t& matrix)
    {
        *this = s;
        transform(matrix);
    }

    void vec_t::transform_point(const matrix_t& matrix)
    {
        vec_t out;

        out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0] + matrix.m[3][0];
        out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1] + matrix.m[3][1];
        out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2] + matrix.m[3][2];
        out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3] + matrix.m[3][3];

        x = out.x;
        y = out.y;
        z = out.z;
        w = out.w;
    }

    void vec_t::transform_vector(const matrix_t& matrix)
    {
        vec_t out;

        out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0];
        out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1];
        out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2];
        out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3];

        x = out.x;
        y = out.y;
        z = out.z;
        w = out.w;
    }

    enum class Color
    {
        DirX,
        DirY,
        DirZ,
        Text,
        Face,
        FaceHighlight,
        Count
    };

    enum class Face
    {
        Right,
        Back,
        Top,
        Left,
        Front,
        Bottom
    };

    struct Style
    {
        Style() {
            // initialize default colors
            Colors[size_t(Color::DirX)] = ImVec4(0.750f, 0.000f, 0.000f, 1.000f);
            Colors[size_t(Color::DirY)] = ImVec4(0.000f, 0.750f, 0.000f, 1.000f);
            Colors[size_t(Color::DirZ)] = ImVec4(0.000f, 0.000f, 0.750f, 1.000f);
            Colors[size_t(Color::Text)] = ImVec4(0.000f, 0.000f, 0.000f, 1.000f);
            Colors[size_t(Color::Face)] = ImVec4(0.808f, 0.808f, 0.800f, 0.800f);
            Colors[size_t(Color::FaceHighlight)] = ImVec4(0.918f, 0.498f, 0.259f, 1.000f);

            axis_labels = {"X", "Y", "Z"};
            // Right, Back, Top, Left, Front, Bottom
            face_labels = {"+X", "+Y", "+Z", "-X", "-Y", "-Z"};
        }

        float translation_line_thickness{3.0f};
        float translation_line_arrow_size{6.0f};

        std::array<ImVec4, size_t(Color::Count)> Colors;
        std::array<std::string, 3> axis_labels;
        std::array<std::string, 6> face_labels;
    };

    struct Context
    {
        ImDrawList* drawlist{nullptr};
        Style style;

        matrix_t view_mat;
        matrix_t projection_mat;
        matrix_t mvp;
        matrix_t view_projection;

        vec_t ray_origin;
        vec_t ray_vector;


        bool animation_running{ false };

        bool is_view_manipulator_hovered{false};
        bool mouse_over{false};
        bool reversed{false};

        bool is_orthographic{false};
        float gizmo_size_clip_space{0.1f};

        float azimuth{ 0.0f };
        float zenith{ 0.0f };
    };

    static Context gContext;

    class Interpolation
    {
    public:
        void set(const vec_t& start_dir, const vec_t& end_dir)
        {
            static const vec_t UNIT_X = make_vect(1.0f, 0.0f, 0.0f);
            static const vec_t UNIT_Z = make_vect(0.0f, 0.0f, 1.0f);

            vec_t start_dir_xy = make_vect(start_dir.x, start_dir.y, 0.0f);
            start_dir_xy.normalize();
            vec_t end_dir_xy = make_vect(end_dir.x, end_dir.y, 0.0f);
            end_dir_xy.normalize();

            m_start_azimuth = std::acos(std::clamp(dot(-start_dir_xy, UNIT_X), -1.0f, 1.0f));
            if (dot(UNIT_Z, cross(UNIT_X, -start_dir_xy)) < 0.0f)
                m_start_azimuth = 2.0f * float(M_PI) - m_start_azimuth;
            m_start_zenith  = std::acos(std::clamp(dot(-start_dir, UNIT_Z), -1.0f, 1.0f));

            float end_zenith = std::acos(std::clamp(dot(-end_dir, UNIT_Z), -1.0f, 1.0f));
            float end_azimuth;
            if (end_dir_xy.y > 0.0f) {
                if (std::abs(end_dir_xy.x) < FLT_EPSILON)
                    end_azimuth = 1.5f * float(M_PI);
                else if (end_dir_xy.x < 0.0f)
                    end_azimuth = 1.75f * float(M_PI);
                else
                    end_azimuth = 1.25f * float(M_PI);
            }
            else {
                end_azimuth = std::acos(std::clamp(dot(-end_dir_xy, UNIT_X), -1.0f, 1.0f));
                if (dot(UNIT_Z, cross(UNIT_X, -end_dir_xy)) < 0.0f)
                    end_azimuth = -end_azimuth;
            }

            float delta_zenith = end_zenith - m_start_zenith;
            float delta_azimuth = end_azimuth - m_start_azimuth;
            if (std::abs(delta_azimuth) > float(M_PI)) {
                if (delta_azimuth > 0.0f)
                    delta_azimuth = -(2.0f * float(M_PI) - delta_azimuth);
                else
                    delta_azimuth = 2.0f * float(M_PI) + delta_azimuth;
            }

            m_step_azimuth = delta_azimuth / float(FRAMES_COUNT);
            m_step_zenith = delta_zenith / float(FRAMES_COUNT);

            gContext.azimuth = m_start_azimuth;
            gContext.zenith = m_start_zenith;

            m_remaining_frames = (m_step_azimuth != 0.0f || m_step_zenith != 0.0f) ? FRAMES_COUNT : -1;
        }

        bool running() const {
            return 0 <= m_remaining_frames;
        }

        void interpolate() {
            if (m_remaining_frames < 0)
                return;

            float curr_frame = float(1 + FRAMES_COUNT - m_remaining_frames);
            gContext.azimuth = m_start_azimuth + curr_frame * m_step_azimuth;
            gContext.zenith = m_start_zenith + curr_frame * m_step_zenith;
            --m_remaining_frames;

//            std::cout << ">>>>> curr_frame: " << curr_frame << " - azimuth: " << rad2deg(gContext.azimuth) << " - zenith : " << rad2deg(gContext.zenith) << "\n";
        }

    private:
        float m_start_azimuth;
        float m_start_zenith;

        float m_step_azimuth;
        float m_step_zenith;

        int m_remaining_frames{-1};

        static constexpr int FRAMES_COUNT = 40;
    };

    static const vec_t direction_unary[3] = { make_vect(1.0f, 0.0f, 0.0f), make_vect(0.0f, 1.0f, 0.0f), make_vect(0.0f, 0.0f, 1.0f) };

    static ImU32 get_color_u32(int idx)
    {
        IM_ASSERT(idx < int(Color::Count));
        return ImGui::ColorConvertFloat4ToU32(gContext.style.Colors[idx]);
    }

    static ImVec2 world_to_pos(const vec_t& world_pos, const matrix_t& mat, const ImVec2& position = ImVec2(0.0f, 0.0f),
        const ImVec2& size = ImVec2(0.0f, 0.0f))
    {
        vec_t trans;
        trans.transform_point(world_pos, mat);
        trans *= 0.5f / trans.w;
        trans += make_vect(0.5f, 0.5f);
        trans.y = 1.f - trans.y;
        trans.x *= size.x;
        trans.y *= size.y;
        trans.x += position.x;
        trans.y += position.y;
        return ImVec2(trans.x, trans.y);
    }

    static void compute_camera_ray(vec_t& ray_origin, vec_t& ray_dir, const ImVec2& position = ImVec2(0.0f, 0.0f),
        const ImVec2& size = ImVec2(0.0f, 0.0f))
    {
        ImGuiIO& io = ImGui::GetIO();

        matrix_t view_proj_inverse;
        view_proj_inverse.inverse(gContext.view_mat * gContext.projection_mat);

        const float mox = ((io.MousePos.x - position.x) / size.x) * 2.0f - 1.0f;
        const float moy = (1.0f - ((io.MousePos.y - position.y) / size.y)) * 2.0f - 1.0f;

        const float zNear = gContext.reversed ? (1.0f - FLT_EPSILON) : 0.0f;
        const float zFar = gContext.reversed ? 0.0f : (1.0f - FLT_EPSILON);

        ray_origin.transform(make_vect(mox, moy, zNear, 1.0f), view_proj_inverse);
        ray_origin *= 1.0f / ray_origin.w;

        vec_t ray_end;
        ray_end.transform(make_vect(mox, moy, zFar, 1.0f), view_proj_inverse);
        ray_end *= 1.0f / ray_end.w;
        ray_dir = normalized(ray_end - ray_origin);
    }

    static float intersect_ray_plane(const vec_t& r_origin, const vec_t& r_vector, const vec_t& plan)
    {
        float numer = plan.dot3(r_origin) - plan.w;
        float denom = plan.dot3(r_vector);

        if (fabsf(denom) < FLT_EPSILON)  // normal is orthogonal to vector, cant intersect
            return -1.0f;

        return -(numer / denom);
    }

    static bool is_hovering_window()
    {
        ImGuiContext& g = *ImGui::GetCurrentContext();
        ImGuiWindow* window = ImGui::FindWindowByName(gContext.drawlist->_OwnerName);
        if (g.HoveredWindow == window)   // Mouse hovering drawlist window
            return true;
        if (g.HoveredWindow != NULL)     // Any other window is hovered
            return false;
        if (ImGui::IsMouseHoveringRect(window->InnerRect.Min, window->InnerRect.Max, false))   // Hovering drawlist window rect, while no other window is hovered (for _NoInputs windows)
            return true;
        return false;
    }

    void set_orthographic(bool is_orthographic)
    {
        gContext.is_orthographic = is_orthographic;
    }

    void set_draw_list(ImDrawList* drawlist)
    {
        gContext.drawlist = drawlist ? drawlist : ImGui::GetWindowDrawList();
    }

    bool is_animation_running()
    {
        return gContext.animation_running;
    }

    bool is_view_manipulate_hovered()
    {
        return gContext.is_view_manipulator_hovered;
    }

    static void compute_context(const float* view, const float* projection)
    {
        float matrix[16] = { 1.0f, 0.0f, 0.0f, 0.0f, 
                             0.0f, 1.0f, 0.0f, 0.0f,
                             0.0f, 0.0f, 1.0f, 0.0f,
                             0.0f, 0.0f, 0.0f, 1.0f };

        gContext.view_mat = *(matrix_t*)view;
        gContext.projection_mat = *(matrix_t*)projection;
        gContext.mouse_over = is_hovering_window();

        gContext.view_projection = gContext.view_mat * gContext.projection_mat;
        gContext.mvp = gContext.view_projection;

        // projection reverse
        vec_t near_pos, far_pos;
        near_pos.transform(make_vect(0.0f, 0.0f, 1.0f, 1.0f), gContext.projection_mat);
        far_pos.transform(make_vect(0.0f, 0.0f, 2.0f, 1.0f), gContext.projection_mat);

        gContext.reversed = near_pos.z / near_pos.w > far_pos.z / far_pos.w;

        compute_camera_ray(gContext.ray_origin, gContext.ray_vector);
    }

    static void view_manipulate(float* view, float length, const ImVec2& position, const ImVec2& size, ImU32 backgroundColor)
    {
        static bool is_clicking = false;
        static vec_t interpolation_dir;

        static Interpolation interpolation;

        matrix_t svg_view = gContext.view_mat;
        matrix_t svg_projection = gContext.projection_mat;

        gContext.drawlist->PushClipRectFullScreen();

        ImGuiIO& io = ImGui::GetIO();
        gContext.drawlist->AddRectFilled(position, position + size, backgroundColor);

        matrix_t view_inverse;
        view_inverse.inverse(*(matrix_t*)view);
        vec_t cam_target = view_inverse.v.position - view_inverse.v.dir * length;

        // view/projection matrices
        float distance = 3.0f;
        matrix_t cube_projection, cube_view;
        if (gContext.is_orthographic)
            orthographic(-1.0f, 1.0f, -1.0f, 1.0f, 0.01f, 1000.0f, cube_projection.m16);
        else {
            float fov = rad2deg(acosf(distance / (sqrtf(distance * distance + 3.f))));
            perspective(fov / sqrtf(2.f), size.x / size.y, 0.01f, 1000.0f, cube_projection.m16);
        }

        vec_t dir = make_vect(view_inverse.m[2][0], view_inverse.m[2][1], view_inverse.m[2][2]);
        vec_t up = make_vect(view_inverse.m[1][0], view_inverse.m[1][1], view_inverse.m[1][2]);
        vec_t eye = dir * distance;
        vec_t zero = make_vect(0.0f, 0.0f);
        look_at(&eye.x, &zero.x, &up.x, cube_view.m16);

        // set context
        gContext.view_mat = cube_view;
        gContext.projection_mat = cube_projection;
        compute_camera_ray(gContext.ray_origin, gContext.ray_vector, position, size);

        matrix_t res = cube_view * cube_projection;

        // panels
        static const ImVec2 panel_position[9] = {
            ImVec2(0.75f,0.75f), ImVec2(0.25f, 0.75f), ImVec2(0.0f, 0.75f),
            ImVec2(0.75f, 0.25f), ImVec2(0.25f, 0.25f), ImVec2(0.0f, 0.25f),
            ImVec2(0.75f, 0.0f), ImVec2(0.25f, 0.0f), ImVec2(0.0f, 0.0f)
        };

        static const ImVec2 panel_size[9] = {
            ImVec2(0.25f,0.25f), ImVec2(0.5f, 0.25f), ImVec2(0.25f, 0.25f),
            ImVec2(0.25f, 0.5f), ImVec2(0.5f, 0.5f), ImVec2(0.25f, 0.5f),
            ImVec2(0.25f, 0.25f), ImVec2(0.5f, 0.25f), ImVec2(0.25f, 0.25f)
        };

        // tag faces
        bool boxes[27]{};
        static int over_box = -1;
        for (int i_pass = 0; i_pass < 2; i_pass++) {
            for (int i_face = 0; i_face < 6; i_face++) {
                int normal_index = (i_face % 3);
                int perp_x_index = (normal_index + 1) % 3;
                int perp_y_index = (normal_index + 2) % 3;
                float invert = (i_face > 2) ? -1.0f : 1.0f;
                vec_t index_vector_x = direction_unary[perp_x_index] * invert;
                vec_t index_vector_y = direction_unary[perp_y_index] * invert;
                vec_t box_origin = direction_unary[normal_index] * -invert - index_vector_x - index_vector_y;

                // plan local space
                vec_t n = direction_unary[normal_index] * invert;
                vec_t view_space_normal = n;
                vec_t view_space_point = n * 0.5f;
                view_space_normal.transform_vector(cube_view);
                view_space_normal.normalize();
                view_space_point.transform_point(cube_view);
                vec_t viewSpaceFacePlan = build_plan(view_space_point, view_space_normal);

                // back face culling
                // Threshold for orthographic camera is needed to avoid unwanted culling of faces
                if (gContext.is_orthographic && viewSpaceFacePlan.w > 0.5f ||
                    !gContext.is_orthographic && viewSpaceFacePlan.w > 0.0f)
                    continue;

                vec_t face_plan = build_plan(n * 0.5f, n);
                float len = intersect_ray_plane(gContext.ray_origin, gContext.ray_vector, face_plan);
                vec_t pos_on_plan = gContext.ray_origin + gContext.ray_vector * len - (n * 0.5f);

                float localx = dot(direction_unary[perp_x_index], pos_on_plan) * invert + 0.5f;
                float localy = dot(direction_unary[perp_y_index], pos_on_plan) * invert + 0.5f;

                // panels
                vec_t dx = direction_unary[perp_x_index];
                vec_t dy = direction_unary[perp_y_index];
                vec_t origin = direction_unary[normal_index] - dx - dy;
                for (int i_panel = 0; i_panel < 9; i_panel++) {
                    vec_t box_coord = box_origin + index_vector_x * float(i_panel % 3) + index_vector_y * float(i_panel / 3) + make_vect(1.0f, 1.0f, 1.0f);
                    ImVec2 p = panel_position[i_panel] * 2.0f;
                    ImVec2 s = panel_size[i_panel] * 2.0f;
                    ImVec2 faceC_coords_screen[4];
                    vec_t panel_pos[4] = { dx * p.x + dy * p.y,
                                           dx * p.x + dy * (p.y + s.y),
                                           dx * (p.x + s.x) + dy * (p.y + s.y),
                                           dx * (p.x + s.x) + dy * p.y };

                    for (unsigned int iCoord = 0; iCoord < 4; iCoord++) {
                        faceC_coords_screen[iCoord] = world_to_pos((panel_pos[iCoord] + origin) * 0.5f * invert, res, position, size);
                    }

                    ImVec2 panel_corners[2] = { panel_position[i_panel], panel_position[i_panel] + panel_size[i_panel] };
                    bool inside_panel = localx > panel_corners[0].x && localx < panel_corners[1].x &&
                        localy > panel_corners[0].y && localy < panel_corners[1].y;
                    int box_coord_int = int(box_coord.x * 9.0f + box_coord.y * 3.0f + box_coord.z);
                    IM_ASSERT(box_coord_int < 27);
                    boxes[box_coord_int] |= inside_panel && gContext.mouse_over;

                    if (i_pass) {
                        // draw faces
                        gContext.drawlist->AddConvexPolyFilled(faceC_coords_screen, 4, get_color_u32(int(Color::Face)));
                        if (boxes[box_coord_int]) {
                            gContext.drawlist->AddConvexPolyFilled(faceC_coords_screen, 4, get_color_u32(int(Color::FaceHighlight)));

                            if (io.MouseDown[0] && !is_clicking && GImGui->ActiveId == 0) {
                                over_box = box_coord_int;
                                is_clicking = true;
                            }
                        }

                        // draw face labels
                        {
                            int vtx_write_start = gContext.drawlist->VtxBuffer.Size;

                            auto label = gContext.style.face_labels[i_face];
                            ImVec2 label_size = ImGui::CalcTextSize(label.c_str());
                            float scale_factor = 2.0f / size.y;
                            auto label_origin = label_size * 0.5f;
                            gContext.drawlist->AddText(ImVec2(0.0f, 0.0f), get_color_u32(int(Color::Text)), label.c_str());
                            ImDrawVert* vtx_write_end = gContext.drawlist->_VtxWritePtr;

                            vec_t tdx = direction_unary[perp_x_index];
                            vec_t tdy = direction_unary[perp_y_index];
                            ImVec2 invert2 = {1, 1};
                            switch (Face(i_face)) {
                            case Face::Right:
                                invert2.y = -1;
                                break;
                            case Face::Back:
                                tdx = direction_unary[0];
                                tdy = direction_unary[2];
                                invert2 = {-1, -1};
                                break;
                            case Face::Top:
                                invert2.y = -1;
                                break;
                            case Face::Left:
                                break;
                            case Face::Front:
                                tdx = direction_unary[0];
                                tdy = direction_unary[2];
                                invert2.x = -1;
                                break;
                            case Face::Bottom:
                                invert2 = {-1, -1};
                                break;
                            }

                            for (auto v = (gContext.drawlist->VtxBuffer.Data + vtx_write_start); v < vtx_write_end; v++) {
                                auto  pp = ((v->pos - label_origin) * scale_factor * invert2 + ImVec2{0.5, 0.5}) * 2.f;
                                vec_t pt = tdx * pp.x + tdy * pp.y;
                                v->pos = world_to_pos((pt + origin) * 0.5 * invert, res, position, size);
                            }
                        }
                    }
                }
            }
        }
        
        // draw axes
        {
            vec_t origin = make_vect(-0.5f, -0.5f, -0.5f);
            for (int i = 0; i < 3; ++i) {
                vec_t dir_axis = direction_unary[i];
                ImVec2 base_sspace = world_to_pos(origin, res, position, size);
                ImVec2 world_dir_sspace = world_to_pos(origin + dir_axis, res, position, size);

                bool visible = false;
                {
                    vec_t mid = origin + (dir_axis * 0.5f);
                    vec_t eye = make_vect(0.0f, 0.0f, 0.5f);
                    eye.normalize();
                    for (int j = 1; j <= 2; j++) {
                        vec_t f = mid + (direction_unary[(i + j) % 3] * 0.5f);
                        f.transform_vector(cube_view);
                        f.normalize();
                        auto w = f.dot(eye);
                        if (w > 0) {
                            visible = true;
                            break;
                        }
                    }
                }

                ImVec4 direction_color_v = gContext.style.Colors[size_t(Color::DirX) + i];
                if (!visible)
                    direction_color_v.w *= 0.3f;

                ImU32 direction_color = ImGui::ColorConvertFloat4ToU32(direction_color_v);
                gContext.drawlist->AddLine(base_sspace, world_dir_sspace, direction_color, gContext.style.translation_line_thickness);
            
                // Arrow head begin
                ImVec2 dir(base_sspace - world_dir_sspace);
            
                float d = sqrtf(ImLengthSqr(dir));
                dir /= d; // Normalize
                dir *= gContext.style.translation_line_arrow_size;
            
                ImVec2 ortogonal_dir(dir.y, -dir.x); // Perpendicular vector
                ImVec2 a(world_dir_sspace + dir);
                gContext.drawlist->AddTriangleFilled(world_dir_sspace - dir, a + ortogonal_dir, a - ortogonal_dir, direction_color);
                // Arrow head end

                // Axis text
                ImVec2 label_sspace = world_to_pos(origin + dir_axis * 1.3f, res, position, size);
                ImVec2 label_size = ImGui::CalcTextSize(gContext.style.axis_labels[i].c_str());
                gContext.drawlist->AddText(label_sspace - label_size * 0.5f, direction_color, gContext.style.axis_labels[i].c_str());
            }
        }

        if (interpolation.running())
            interpolation.interpolate();

        gContext.is_view_manipulator_hovered = gContext.mouse_over && ImRect(position, position + size).Contains(io.MousePos);

        if (io.MouseDown[0] && (fabsf(io.MouseDelta[0]) || fabsf(io.MouseDelta[1])) && is_clicking)
            is_clicking = false;

        if (!io.MouseDown[0]) {
            if (is_clicking) {
                // apply new view direction
                int cx = over_box / 9;
                int cy = (over_box - cx * 9) / 3;
                int cz = over_box % 3;
                interpolation_dir = make_vect(1.0f - (float)cx, 1.0f - (float)cy, 1.0f - (float)cz);
                interpolation_dir.normalize();

                if (dot(interpolation_dir, dir) <= 1.0f - 0.01f)
                    interpolation.set(dir, interpolation_dir);
            }
            is_clicking = false;
        }

        gContext.animation_running = interpolation.running();
        if (is_clicking || gContext.animation_running || gContext.is_view_manipulator_hovered)
            ImGui::SetNextFrameWantCaptureMouse(true);

        // restore view/projection because it was used to compute ray
        compute_context(svg_view.m16, svg_projection.m16);

        gContext.drawlist->PopClipRect();
    }

    void view_manipulate(float* view, const float* projection, float length, const ImVec2& position, const ImVec2& size,
        ImU32 backgroundColor)
    {
        // Scale is always local or matrix will be skewed when applying world scale or oriented matrix
        compute_context(view, projection);
        view_manipulate(view, length, position, size, backgroundColor);
    }

    std::pair<float, float> get_azimuth_and_zenith()
    {
        return std::make_pair(gContext.azimuth, gContext.zenith);
    }

} // namespace Slic3r::App::Imgui::NavCube
