#include <bits/stdc++.h>
#include "json.hpp"  
using namespace std;
using nlohmann::json;
class Vector4 {
    public:
        double x, y, z, w;
        // Constructor with default w = 1 for points (homogeneous coordinates)
        Vector4(double _x=0, double _y=0, double _z=0, double _w=1.0)
            : x(_x), y(_y), z(_z), w(_w) {}
    };
    
    // 4x4 matrix class for transformations
    class Matrix4 {
    public:
        double m[4][4];
        // Default constructor (does not initialize for performance)
        Matrix4() { // initialize identity by default
            for(int i=0;i<4;++i) for(int j=0;j<4;++j) m[i][j] = (i==j ? 1.0 : 0.0);
        }
        // Static method to create an identity matrix
        static Matrix4 identity() {
            Matrix4 I;
            for(int i = 0; i < 4; ++i) {
                for(int j = 0; j < 4; ++j) {
                    I.m[i][j] = (i == j ? 1.0 : 0.0);
                }
            }
            return I;
        }
        // Static method to create a translation matrix
        static Matrix4 translation(double tx, double ty, double tz) {
            Matrix4 T = Matrix4::identity();
            T.m[0][3] = tx;
            T.m[1][3] = ty;
            T.m[2][3] = tz;
            return T;
        }
        // Static method to create a scaling matrix
        static Matrix4 scaling(double sx, double sy, double sz) {
            Matrix4 S = Matrix4::identity();
            S.m[0][0] = sx;
            S.m[1][1] = sy;
            S.m[2][2] = sz;
            return S;
        }
        // Static method to create a rotation matrix around an axis by an angle (degrees)
        static Matrix4 rotation(double angleDeg, double ax, double ay, double az) {
            Matrix4 R = Matrix4::identity();
            // Normalize the axis vector (ax, ay, az)
            double len = sqrt(ax*ax + ay*ay + az*az);
            if (len < 1e-9) {
                // If axis vector is zero, return identity (no rotation)
                return R;
            }
            double ux = ax / len;
            double uy = ay / len;
            double uz = az / len;
            double theta = angleDeg * acos(-1.0) / 180.0; // convert degrees to radians
            double c = cos(theta);
            double s = sin(theta);
            double one_c = 1.0 - c;
            // Compute rotation matrix components (Rodrigues' formula)
            R.m[0][0] = c + ux*ux*one_c;
            R.m[0][1] = ux*uy*one_c - uz*s;
            R.m[0][2] = ux*uz*one_c + uy*s;
            R.m[0][3] = 0.0;
            R.m[1][0] = uy*ux*one_c + uz*s;
            R.m[1][1] = c + uy*uy*one_c;
            R.m[1][2] = uy*uz*one_c - ux*s;
            R.m[1][3] = 0.0;
            R.m[2][0] = uz*ux*one_c - uy*s;
            R.m[2][1] = uz*uy*one_c + ux*s;
            R.m[2][2] = c + uz*uz*one_c;
            R.m[2][3] = 0.0;
            R.m[3][0] = 0.0;
            R.m[3][1] = 0.0;
            R.m[3][2] = 0.0;
            R.m[3][3] = 1.0;
            return R;
        }
        // Matrix multiplication (this * other)
        Matrix4 operator*(const Matrix4 &other) const {
            Matrix4 result;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    result.m[i][j] = 0.0;
                    for (int k = 0; k < 4; ++k) {
                        result.m[i][j] += m[i][k] * other.m[k][j];
                    }
                }
            }
            return result;
        }
        // Multiply matrix with a Vector4 (matrix * vector)
        Vector4 operator*(const Vector4 &v) const {
            Vector4 res;
            res.x = m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3]*v.w;
            res.y = m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3]*v.w;
            res.z = m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]*v.w;
            res.w = m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3]*v.w;
            return res;
        }
    };

    // Transform a 4D vertex by a 4x4 matrix (column vector multiplication)
Vector4 applyTransform(const Vector4& v, const Matrix4& mat) {
    Vector4 result;
    result.x = mat.m[0][0]*v.x + mat.m[0][1]*v.y + mat.m[0][2]*v.z + mat.m[0][3]*v.w;
    result.y = mat.m[1][0]*v.x + mat.m[1][1]*v.y + mat.m[1][2]*v.z + mat.m[1][3]*v.w;
    result.z = mat.m[2][0]*v.x + mat.m[2][1]*v.y + mat.m[2][2]*v.z + mat.m[2][3]*v.w;
    result.w = mat.m[3][0]*v.x + mat.m[3][1]*v.y + mat.m[3][2]*v.z + mat.m[3][3]*v.w;
    return result;
}
    