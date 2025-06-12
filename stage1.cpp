#include"helper/helper.hpp"

const int MAX_STACK_SIZE = 32; // maximum allowed stack depth

// 4-dimensional vector (homogeneous coordinates)

int main(int argc, char *argv[]) {
    ios::sync_with_stdio(false);
    cin.tie(NULL);

    // Open input scene file and output file for Stage1
    string inputFile=argv[2];
    string testFolder=argv[1];
    cout<<inputFile<<endl;
    ifstream input(inputFile);
    if (!input.is_open()) {
        cerr << "Error: Could not open input file \"scene.txt\"." << endl;
        return EXIT_FAILURE;
    }
    ofstream output(testFolder+"/output/stage1.txt");
    if (!output.is_open()) {
        cerr << "Error: Could not open output file \"stage1.txt\"." << endl;
        return EXIT_FAILURE;
    }

    // Configure output formatting for floating point numbers
    output.setf(ios::fixed);
    output.precision(7);

    // Read camera parameters (eye, look, up) and projection parameters (fovY, aspect, near, far)
    // These values are read from the file but not used in Stage 1 (they will be used in later stages).
    double eyeX, eyeY, eyeZ;
    double lookX, lookY, lookZ;
    double upX, upY, upZ;
    double fovY, aspectRatio, nearPlane, farPlane;
    if (!(input >> eyeX >> eyeY >> eyeZ)) {
        cerr << "Error: Failed to read camera eye coordinates." << endl;
        return EXIT_FAILURE;
    }
    if (!(input >> lookX >> lookY >> lookZ)) {
        cerr << "Error: Failed to read camera look coordinates." << endl;
        return EXIT_FAILURE;
    }
    if (!(input >> upX >> upY >> upZ)) {
        cerr << "Error: Failed to read camera up vector." << endl;
        return EXIT_FAILURE;
    }
    if (!(input >> fovY >> aspectRatio >> nearPlane >> farPlane)) {
        cerr << "Error: Failed to read projection parameters." << endl;
        return EXIT_FAILURE;
    }
    json j;

// group the data in a way that’s easy to reload later
j["camera"] = {
    { "eye",  { eyeX,  eyeY,  eyeZ  } },
    { "look", { lookX, lookY, lookZ } },
    { "up",   { upX,   upY,   upZ   } }
};

j["projection"] = {
    { "fovY",        fovY        },
    { "aspectRatio", aspectRatio },
    { "near",        nearPlane   },
    { "far",         farPlane    }
};
std::ofstream jsonOut(testFolder+"/data.json");
if (!jsonOut.is_open()) {
    std::cerr << "Error: Could not open stage1.json for writing\n";
    return EXIT_FAILURE;
}
jsonOut << j.dump(4);   // 4-space indentation for readability
jsonOut.close();

    // Initialize the transformation stack with the identity matrix
    vector<Matrix4> matrixStack;
    matrixStack.push_back(Matrix4::identity());

    string command;
    // Process transformation and geometry commands
    while (input >> command) {
        if (command == "end") {
            // End of scene description
            break;
        } else if (command == "triangle") {
            // Read triangle vertex coordinates
            double x1, y1, z1;
            double x2, y2, z2;
            double x3, y3, z3;
            input >> x1 >> y1 >> z1;
            input >> x2 >> y2 >> z2;
            input >> x3 >> y3 >> z3;
            // Create Vector4 for each vertex (w = 1 for points)
            Vector4 v1(x1, y1, z1, 1.0);
            Vector4 v2(x2, y2, z2, 1.0);
            Vector4 v3(x3, y3, z3, 1.0);
            // Apply current transformation to each vertex
            Matrix4 &CTM = matrixStack.back(); // current transformation matrix (top of stack)
            Vector4 tv1 = CTM * v1;
            Vector4 tv2 = CTM * v2;
            Vector4 tv3 = CTM * v3;
            // Output the transformed vertices
            output << tv1.x << " " << tv1.y << " " << tv1.z << "\n";
            output << tv2.x << " " << tv2.y << " " << tv2.z << "\n";
            output << tv3.x << " " << tv3.y << " " << tv3.z << "\n";
            output << "\n"; // blank line after each triangle
        } else if (command == "translate") {
            // Translation: read translation distances
            double tx, ty, tz;
            input >> tx >> ty >> tz;
            // Update the current matrix by post-multiplying with the translation matrix
            Matrix4 T = Matrix4::translation(tx, ty, tz);
            Matrix4 &CTM = matrixStack.back();
            CTM = CTM * T;
        } else if (command == "scale") {
            // Scaling: read scale factors
            double sx, sy, sz;
            input >> sx >> sy >> sz;
            Matrix4 S = Matrix4::scaling(sx, sy, sz);
            Matrix4 &CTM = matrixStack.back();
            CTM = CTM * S;
        } else if (command == "rotate") {
            // Rotation: read angle and axis
            double angleDeg, ax, ay, az;
            input >> angleDeg >> ax >> ay >> az;
            Matrix4 R = Matrix4::rotation(angleDeg, ax, ay, az);
            Matrix4 &CTM = matrixStack.back();
            CTM = CTM * R;
        } else if (command == "push") {
            // Push: duplicate the current top matrix and push onto stack
            if ((int)matrixStack.size() >= MAX_STACK_SIZE) {
                cerr << "Error: Matrix stack overflow (depth > " << MAX_STACK_SIZE << ")." << endl;
                return EXIT_FAILURE;
            }
            matrixStack.push_back(matrixStack.back());
        } else if (command == "pop") {
            // Pop: remove the top matrix (restoring the previous transformation)
            if (matrixStack.size() <= 1) {
                cerr << "Error: Matrix stack underflow (cannot pop base matrix)." << endl;
                return EXIT_FAILURE;
            }
            matrixStack.pop_back();
        } else {
            // Unrecognized command (skip it)
            cerr << "Warning: Ignoring unknown command \"" << command << "\"." << endl;
        }
    }

    input.close();
    output.close();
    return 0;
}
