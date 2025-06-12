#include <bits/stdc++.h>
#include"helper/helper.hpp"
using namespace std;



// Compute an orthonormal basis (r, u, l) from camera parameters and return the View transformation matrix
Matrix4 computeViewMatrix(const Vector4& eye, const Vector4& look, const Vector4& upVec) {
    // 1. Compute forward (l), right (r), and up (u) unit vectors for the camera
    // forward l = normalized(look - eye)
    Vector4 l;
    l.x = look.x - eye.x;
    l.y = look.y - eye.y;
    l.z = look.z - eye.z;
    // normalize l
    double len = sqrt(l.x*l.x + l.y*l.y + l.z*l.z);
    if(len == 0) {
        throw runtime_error("View matrix error: eye and look are the same point (zero forward vector).");
    }
    l.x /= len; l.y /= len; l.z /= len;

    // right r = normalized(cross(l, up))
    // cross(l, up) = (l.y*up.z - l.z*up.y,  l.z*up.x - l.x*up.z,  l.x*up.y - l.y*up.x)
    Vector4 r;
    r.x = l.y*upVec.z - l.z*upVec.y;
    r.y = l.z*upVec.x - l.x*upVec.z;
    r.z = l.x*upVec.y - l.y*upVec.x;
    len = sqrt(r.x*r.x + r.y*r.y + r.z*r.z);
    if(len == 0) {
        throw runtime_error("View matrix error: 'up' vector is parallel to viewing direction.");
    }
    r.x /= len; r.y /= len; r.z /= len;

    // true up u = cross(r, l) (already unit-length if r and l are unit and perpendicular)
    Vector4 u;
    u.x = r.y*l.z - r.z*l.y;
    u.y = r.z*l.x - r.x*l.z;
    u.z = r.x*l.y - r.y*l.x;
    // (u should automatically be unit-length here)

    // 2. Construct rotation matrix (camera orientation)
    Matrix4 R;
    R.m[0][0] = r.x;   R.m[0][1] = r.y;   R.m[0][2] = r.z;   R.m[0][3] = 0;
    R.m[1][0] = u.x;   R.m[1][1] = u.y;   R.m[1][2] = u.z;   R.m[1][3] = 0;
    R.m[2][0] = -l.x;  R.m[2][1] = -l.y;  R.m[2][2] = -l.z;  R.m[2][3] = 0;
    R.m[3][0] = 0;     R.m[3][1] = 0;     R.m[3][2] = 0;     R.m[3][3] = 1;

    // 3. Construct translation matrix to move the world by -eye
    Matrix4 T;
    T.m[0][0] = 1; T.m[0][1] = 0; T.m[0][2] = 0; T.m[0][3] = -eye.x;
    T.m[1][0] = 0; T.m[1][1] = 1; T.m[1][2] = 0; T.m[1][3] = -eye.y;
    T.m[2][0] = 0; T.m[2][1] = 0; T.m[2][2] = 1; T.m[2][3] = -eye.z;
    T.m[3][0] = 0; T.m[3][1] = 0; T.m[3][2] = 0; T.m[3][3] = 1;

    // 4. Compute View Matrix = R * T
    Matrix4 view;
    // Matrix multiplication R * T (both 4x4)
    for(int i=0;i<4;++i){
        for(int j=0;j<4;++j){
            view.m[i][j] = 0;
            for(int k=0;k<4;++k){
                view.m[i][j] += R.m[i][k] * T.m[k][j];
            }
        }
    }
    return view;
}



int main() { 
    json j;
    ifstream("data.json")>> j;
    auto cam          = j.at("camera");           // throws if key missing
    auto eyeArr       = cam.at("eye");            // [x,y,z]
    auto lookArr      = cam.at("look");
    auto upArr        = cam.at("up");
    Vector4 eye (eyeArr [0],  eyeArr [1],  eyeArr [2],  1.0);
    Vector4 look(lookArr[0], lookArr[1], lookArr[2],  1.0);
    Vector4 up  (upArr [0],  upArr [1],  upArr [2],  0.0);  // direction ⇒ w = 0

    // Compute View Transformation matrix
    Matrix4 viewMatrix;
    try {
        viewMatrix = computeViewMatrix(eye, look, up);
    } catch (const exception& ex) {
        cerr << ex.what() << endl;
        return EXIT_FAILURE;
    }

    // Open stage1.txt for reading and stage2.txt for writing
    ifstream stage1File("output/stage1.txt");
    ofstream stage2File("output/stage2.txt");
    if(!stage1File.is_open() || !stage2File.is_open()) {
        cerr << "Error: Could not open stage1.txt or stage2.txt file." << endl;
        return EXIT_FAILURE;
    }
    stage2File << fixed << setprecision(7);

    // Read vertices from stage1, apply view transform, and write to stage2
    string line;
    int vertexCount = 0;
    vector<Vector4> triangleVerts;
    triangleVerts.reserve(3);
    while(true) {
        // Try reading a vertex line
        double vx, vy, vz;
        if(!(stage1File >> vx >> vy >> vz)) {
            break; // no more vertices (or EOF)
        }
        // Each vertex read:
        Vector4 vWorld(vx, vy, vz, 1.0);
        // Apply view matrix to get eye-space vertex
        Vector4 vEye = applyTransform(vWorld, viewMatrix);
        // Write transformed vertex to stage2 file
        stage2File << vEye.x << " " << vEye.y << " " << vEye.z << endl;
        vertexCount++;
        triangleVerts.push_back(vEye);

        // If we have written 3 vertices (one triangle), add a blank line and reset counter
        if(vertexCount % 3 == 0) {
            stage2File << endl;
            triangleVerts.clear();
        }
    }
    stage1File.close();
    stage2File.close();

   
    
    return 0;
}

