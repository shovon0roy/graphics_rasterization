#include"helper/helper.hpp"
using namespace std;

int main(){
    json j;
    ifstream("data.json")>> j;
    auto projection = j.at("projection"); // throws if key missing
    double fovY=projection.at("fovY"), aspectRatio=projection.at("aspectRatio"), near=projection.at("near"), far=projection.at("far");
  
    
    if(near <= 0 || far <= 0 || far <= near) {
        cerr << "Error: Invalid near/far values (must be positive and far > near)." << endl;
        return EXIT_FAILURE;
    }
    if(fovY <= 0 || fovY >= 180) {
        cerr << "Error: Field of view (fovY) must be between 0 and 180 degrees." << endl;
        return EXIT_FAILURE;
    }
    if(aspectRatio <= 0) {
        cerr << "Error: Aspect ratio must be positive." << endl;
        return EXIT_FAILURE;
    }

    // Compute the Projection matrix
    Matrix4 projMatrix;
    // Convert fovY to radians for math functions
    double fovYrad = fovY * M_PI / 180.0;
    double tanHalfFovY = tan(fovYrad / 2.0);
    // Elements for projection matrix (based on symmetric frustum)
    projMatrix.m[0][0] = 1.0 / (aspectRatio * tanHalfFovY);
    projMatrix.m[0][1] = 0; 
    projMatrix.m[0][2] = 0; 
    projMatrix.m[0][3] = 0;

    projMatrix.m[1][0] = 0;
    projMatrix.m[1][1] = 1.0 / tanHalfFovY;
    projMatrix.m[1][2] = 0;
    projMatrix.m[1][3] = 0;

    projMatrix.m[2][0] = 0;
    projMatrix.m[2][1] = 0;
    projMatrix.m[2][2] = -(far + near) / (far - near);
    projMatrix.m[2][3] = -(2 * far * near) / (far - near);

    projMatrix.m[3][0] = 0;
    projMatrix.m[3][1] = 0;
    projMatrix.m[3][2] = -1;  // note: this is the value that will multiply z_e to produce w'
    projMatrix.m[3][3] = 0;

    // Open stage2.txt for reading and stage3.txt for writing
    ifstream stage2File("output/stage2.txt");
    ofstream stage3File("output/stage3.txt");
    if(!stage2File.is_open() || !stage3File.is_open()) {
        cerr << "Error: Could not open stage2.txt or stage3.txt file." << endl;
        return EXIT_FAILURE;
    }
    stage3File << fixed << setprecision(7);

    // Read eye-space vertices from stage2, apply projection, and write to stage3
  int  vertexCount = 0;
    while(true) {
        double vx, vy, vz;
        if(!(stage2File >> vx >> vy >> vz)) {
            break; // EOF or no more data
        }
        Vector4 vEye(vx, vy, vz, 1.0);
        // Apply projection matrix
        Vector4 vClip = applyTransform(vEye, projMatrix);
        // Perform perspective division to get normalized device coordinates
        if(vClip.w == 0) {
            cerr << "Warning: w=0 during perspective division." << endl;
            // (In a well-defined frustum, w should never be 0 for points in front of the camera.)
            // We skip or handle this gracefully.
            continue;
        }
        double ndcX = vClip.x / vClip.w;
        double ndcY = vClip.y / vClip.w;
        double ndcZ = vClip.z / vClip.w;
        // Write NDC coordinates to stage3.txt
        stage3File << ndcX << " " << ndcY << " " << ndcZ << endl;
        vertexCount++;
        if(vertexCount % 3 == 0) {
            stage3File << endl;
        }
    }
    stage2File.close();
    stage3File.close();

    cout << "Pipeline Stage 2 and Stage 3 completed successfully." << endl;
    return 0; 
}