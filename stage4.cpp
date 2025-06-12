#include <bits/stdc++.h>
#include "helper/bitmap_image.hpp"
using namespace std;

// Structure to hold a 2D point with depth (after projection to screen)
struct Vertex {
    double x; 
    double y; 
    double z;
};
struct RGB { unsigned char r, g, b; };

RGB hsv2rgb(double h, double s, double v)
{
    /* h in [0,360), s and v in [0,1] */
    double c = v * s;
    double x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
    double m = v - c;

    double r1=0, g1=0, b1=0;
    if      (h <  60) { r1 = c; g1 = x; }
    else if (h < 120) { r1 = x; g1 = c; }
    else if (h < 180) { g1 = c; b1 = x; }
    else if (h < 240) { g1 = x; b1 = c; }
    else if (h < 300) { r1 = x; b1 = c; }
    else              { r1 = c; b1 = x; }

    return RGB{
        static_cast<unsigned char>((r1 + m) * 255.0 + 0.5),
        static_cast<unsigned char>((g1 + m) * 255.0 + 0.5),
        static_cast<unsigned char>((b1 + m) * 255.0 + 0.5)
    };
}

RGB distinctColor(int idx)
{
    /* golden-angle in degrees ≈ 360 * (φ − 1) */
    constexpr double GOLDEN_ANGLE = 137.50776405003785;
    double hue = fmod(idx * GOLDEN_ANGLE, 360.0);   // wrap at 360
    return hsv2rgb(hue, 1.0, 1.0);                  // full-sat, full-val
}
// Function to map NDC coordinates to pixel coordinates
// Takes NDC (x_ndc, y_ndc) and maps to pixel (px, py) given viewport and resolution.
void mapToPixel(double x_ndc, double y_ndc, int screen_width, int screen_height, 
                double x_left, double x_right, double y_bottom, double y_top,
                double &out_px, double &out_py) {
    // Map x from [x_left, x_right] -> [0, screen_width-1]
    out_px = (x_ndc - x_left) / (x_right - x_left) * (screen_width - 1);
    // Map y from [y_bottom, y_top] -> [screen_height-1, 0] (note the inversion for y)
    out_py = (y_top - y_ndc) / (y_top - y_bottom) * (screen_height - 1);
}

// Helper function to interpolate values linearly.
// Given two points (y1, val1) and (y2, val2), returns the interpolated value at y.
double interpolate(double y, double y1, double val1, double y2, double val2) {
    if(fabs(y2 - y1) < 1e-9) {
        return val1; // Avoid division by zero if points have the same y (shouldn't usually happen for interpolation)
    }
    double t = (y - y1) / (y2 - y1);
    return val1 + t * (val2 - val1);
}

int main(int argc, char *argv[]) {
    ios::sync_with_stdio(false);
    cin.tie(NULL);

    // --- Read configuration (viewport and resolution) ---
    string inputFile =argv[2];
    string testFolder=argv[1];
    ifstream configFile(inputFile);
    if(!configFile.is_open()) {
        cerr << "Error: Could not open"+inputFile << endl;
        return EXIT_FAILURE;
    }
    int screen_width, screen_height;
    double x_left, x_right, y_bottom, y_top;
    double z_near, z_far;
    configFile >> screen_width >> screen_height;
    configFile >> x_left;
    configFile >> y_bottom;
    configFile >> z_near >> z_far;
    configFile.close();
     x_right = -x_left;
     y_top   = -y_bottom ;
    

    // Allocate and initialize Z-buffer
    vector<double> zbuffer(screen_width * screen_height, z_far);

    // Prepare output image (initialize all pixels to black)
    bitmap_image image(screen_width, screen_height);
    image.set_all_channels(0, 0, 0);  // set background to black

    // Open stage3 vertex data
    ifstream stage3File(testFolder+"/output/stage3.txt");
    if(!stage3File.is_open()) {
        cerr << "Error: Could not open output/stage3.txt" << endl;
        return EXIT_FAILURE;
    }

    // Variables to hold one triangle's vertices in NDC
    double x_ndc, y_ndc, z_ndc;
    vector<Vertex> triNDC;
    triNDC.reserve(3);

    int triangleIndex = 0;
    // Read vertices from stage3 file, 3 lines per triangle
    while(true) {
        triNDC.clear();
        // Attempt to read three vertices for a triangle
        for(int v = 0; v < 3; ++v) {
            if(!(stage3File >> x_ndc >> y_ndc >> z_ndc)) {
                break; // EOF or incomplete triangle
            }
            triNDC.push_back({x_ndc, y_ndc, z_ndc});
        }
        if(triNDC.size() < 3) {
            break; // End of file (or a partial triangle, which we ignore)
        }

        // Map NDC vertices to screen pixel coordinates
        Vertex v0, v1, v2;
        mapToPixel(triNDC[0].x, triNDC[0].y, screen_width, screen_height, 
                   x_left, x_right, y_bottom, y_top, v0.x, v0.y);
        v0.z = triNDC[0].z;
        mapToPixel(triNDC[1].x, triNDC[1].y, screen_width, screen_height, 
                   x_left, x_right, y_bottom, y_top, v1.x, v1.y);
        v1.z = triNDC[1].z;
        mapToPixel(triNDC[2].x, triNDC[2].y, screen_width, screen_height, 
                   x_left, x_right, y_bottom, y_top, v2.x, v2.y);
        v2.z = triNDC[2].z;

        // Sort vertices by y (ascending order)
        Vertex vt = v0, vm = v1, vb = v2; // vt = top, vm = middle, vb = bottom
        // sort such that vt.y <= vm.y <= vb.y
        if(vt.y > vm.y) swap(vt, vm);
        if(vt.y > vb.y) swap(vt, vb);
        if(vm.y > vb.y) swap(vm, vb);

        // Determine unique color for this triangle (using triangleIndex)
        RGB col = distinctColor(triangleIndex);
        unsigned char r = col.r, g = col.g, b = col.b;
        triangleIndex++;

        // Calculate some values for interpolation along edges
        // Avoid division by zero for horizontal edges by checking differences
        double invSlope1 = 0, invSlope2 = 0;
        double invSlope1_z = 0, invSlope2_z = 0;
        bool halfTriangle = false; // will indicate if we have a flat-top or flat-bottom case

        // If the triangle is degenerate (zero area), skip it
        double area = (vt.x*(vm.y - vb.y) + vm.x*(vb.y - vt.y) + vb.x*(vt.y - vm.y));
        if(fabs(area) < 1e-9) {
            continue; // no area to rasterize (this likely won't happen unless input has duplicate vertices or line)
        }

        // If flat-bottom (vm.y == vb.y) or flat-top (vt.y == vm.y), we can handle accordingly
        if(fabs(vm.y - vb.y) < 1e-9) {
            // Flat-bottom triangle: vm and vb are at same y-level
            halfTriangle = true;
            // Ensure vt is top, vm and vb form the bottom edge
            // Determine which of vm or vb is left/right by x
            Vertex vleft = vm, vright = vb;
            if(vb.x < vm.x) swap(vleft, vright);
            // Iterate from y = vt.y to y = vm.y (which is vb.y)
            int y_start = (int)ceil(vt.y);
            int y_end   = (int)floor(vm.y);
            for(int y = y_start; y <= y_end; ++y) {
                if(y < 0 || y >= screen_height) continue; // skip scanlines outside screen
                // Compute intersections with left and right edges at this y
                double x_l = interpolate(y + 0.5, vt.y, vt.x, vleft.y, vleft.x);
                double x_r = interpolate(y + 0.5, vt.y, vt.x, vright.y, vright.x);
                double z_l = interpolate(y + 0.5, vt.y, vt.z, vleft.y, vleft.z);
                double z_r = interpolate(y + 0.5, vt.y, vt.z, vright.y, vright.z);
                if(x_l > x_r) swap(x_l, x_r), swap(z_l, z_r);
                // Determine pixel column bounds (clamp to [0, width-1])
                int x_start = (int)ceil(x_l);
                int x_end   = (int)floor(x_r);
                if(x_start < 0) x_start = 0;
                if(x_end >= screen_width) x_end = screen_width - 1;
                // Fill pixels between x_start and x_end
                for(int x = x_start; x <= x_end; ++x) {
                    // Interpolate depth for this pixel
                    double z_pixel;
                    if(x_l == x_r) {
                        z_pixel = z_l;  // edge case: very narrow triangle span
                    } else {
                        z_pixel = interpolate(x + 0.5, x_l, z_l, x_r, z_r);
                    }
                    // Depth test: update if closer (smaller Z) than current
                    int bufferIndex = y * screen_width + x;
                    if(z_pixel < zbuffer[bufferIndex]) {
                        zbuffer[bufferIndex] = z_pixel;
                        image.set_pixel(x, y, r, g, b);
                    }
                }
            }
        }
        if(fabs(vt.y - vm.y) < 1e-9) {
            // Flat-top triangle: vt and vm are at same y-level
            halfTriangle = true;
            // vt and vm form the top edge
            // Determine left vs right by x
            Vertex vleft = vt, vright = vm;
            if(vm.x < vt.x) swap(vleft, vright);
            // Iterate from y = vt.y (which is vm.y) down to vb.y
            int y_start = (int)ceil(vt.y);
            int y_end   = (int)floor(vb.y);
            for(int y = y_start; y <= y_end; ++y) {
                if(y < 0 || y >= screen_height) continue;
                // Intersections with left and right edges (now edges are from top vertices to bottom vertex)
                double x_l = interpolate(y + 0.5, vleft.y, vleft.x, vb.y, vb.x);
                double x_r = interpolate(y + 0.5, vright.y, vright.x, vb.y, vb.x);
                double z_l = interpolate(y + 0.5, vleft.y, vleft.z, vb.y, vb.z);
                double z_r = interpolate(y + 0.5, vright.y, vright.z, vb.y, vb.z);
                if(x_l > x_r) swap(x_l, x_r), swap(z_l, z_r);
                int x_start = (int)ceil(x_l);
                int x_end   = (int)floor(x_r);
                if(x_start < 0) x_start = 0;
                if(x_end >= screen_width) x_end = screen_width - 1;
                for(int x = x_start; x <= x_end; ++x) {
                    double z_pixel;
                    if(x_l == x_r) {
                        z_pixel = z_l;
                    } else {
                        z_pixel = interpolate(x + 0.5, x_l, z_l, x_r, z_r);
                    }
                    int bufferIndex = y * screen_width + x;
                    if(z_pixel < zbuffer[bufferIndex]) {
                        zbuffer[bufferIndex] = z_pixel;
                        image.set_pixel(x, y, r, g, b);
                    }
                }
            }
        }
        if(!halfTriangle) {
            // General triangle: split into flat-bottom and flat-top at vm.y
            // Find the splitting vertex (intersection of line from vt to vb at y = vm.y)
            double split_x = interpolate(vm.y, vt.y, vt.x, vb.y, vb.x);
            double split_z = interpolate(vm.y, vt.y, vt.z, vb.y, vb.z);
            Vertex v_split = { split_x, vm.y, split_z };

            // Now we have two triangles: (vt, vm, v_split) is flat-bottom, (vm, v_split, vb) is flat-top
            // Rasterize the flat-bottom triangle (vt, vm, v_split)
            Vertex vleft = vm, vright = v_split;
            if(v_split.x < vm.x) swap(vleft, vright);
            int y_start = (int)ceil(vt.y);
            int y_end   = (int)floor(vm.y);
            for(int y = y_start; y <= y_end; ++y) {
                if(y < 0 || y >= screen_height) continue;
                double x_l = interpolate(y + 0.5, vt.y, vt.x, vleft.y, vleft.x);
                double x_r = interpolate(y + 0.5, vt.y, vt.x, vright.y, vright.x);
                double z_l = interpolate(y + 0.5, vt.y, vt.z, vleft.y, vleft.z);
                double z_r = interpolate(y + 0.5, vt.y, vt.z, vright.y, vright.z);
                if(x_l > x_r) swap(x_l, x_r), swap(z_l, z_r);
                int x_start = (int)ceil(x_l);
                int x_end   = (int)floor(x_r);
                if(x_start < 0) x_start = 0;
                if(x_end >= screen_width) x_end = screen_width - 1;
                for(int x = x_start; x <= x_end; ++x) {
                    double z_pixel;
                    if(x_l == x_r) {
                        z_pixel = z_l;
                    } else {
                        z_pixel = interpolate(x + 0.5, x_l, z_l, x_r, z_r);
                    }
                    int bufferIndex = y * screen_width + x;
                    if(z_pixel < zbuffer[bufferIndex]) {
                        zbuffer[bufferIndex] = z_pixel;
                        image.set_pixel(x, y, r, g, b);
                    }
                }
            }
            // Rasterize the flat-top triangle (vm, v_split, vb)
            vleft = vm; vright = v_split;
            if(v_split.x < vm.x) swap(vleft, vright);
            y_start = (int)ceil(vm.y);
            if(y_start < 0) y_start = 0;
            // We include the horizontal line at y=vm in one of the halves (preferably bottom half to avoid double filling)
            int y_end_top = (int)floor(vb.y);
            for(int y = y_start; y <= y_end_top; ++y) {
                if(y < 0 || y >= screen_height) continue;
                double x_l = interpolate(y + 0.5, vleft.y, vleft.x, vb.y, vb.x);
                double x_r = interpolate(y + 0.5, vright.y, vright.x, vb.y, vb.x);
                double z_l = interpolate(y + 0.5, vleft.y, vleft.z, vb.y, vb.z);
                double z_r = interpolate(y + 0.5, vright.y, vright.z, vb.y, vb.z);
                if(x_l > x_r) swap(x_l, x_r), swap(z_l, z_r);
                int x_start = (int)ceil(x_l);
                int x_end   = (int)floor(x_r);
                if(x_start < 0) x_start = 0;
                if(x_end >= screen_width) x_end = screen_width - 1;
                for(int x = x_start; x <= x_end; ++x) {
                    double z_pixel;
                    if(x_l == x_r) {
                        z_pixel = z_l;
                    } else {
                        z_pixel = interpolate(x + 0.5, x_l, z_l, x_r, z_r);
                    }
                    int bufferIndex = y * screen_width + x;
                    if(z_pixel < zbuffer[bufferIndex]) {
                        zbuffer[bufferIndex] = z_pixel;
                        image.set_pixel(x, y, r, g, b);
                    }
                }
            }
        }
    } // end while reading triangles

    stage3File.close();

    // Save depth buffer to output/z_buffer.txt with 6 decimal precision
    ofstream zFile(testFolder+"/output/z_buffer.txt");
    if(!zFile.is_open()) {
        cerr << "Error: Could not open output/z_buffer.txt for writing." << endl;
        // Even if depth file fails, attempt to save image, then exit
    }
    zFile << fixed << setprecision(6);
    for(int row = 0; row < screen_height; ++row) {
        for(int col = 0; col < screen_width; ++col) {
            double depthVal = zbuffer[row * screen_width + col];
            zFile << depthVal;
            if(col < screen_width - 1) zFile << " ";
        }
        zFile << "\n";
    }
    zFile.close();

    // Save the output image as BMP
    image.save_image(testFolder+"/output/out.bmp");

    return 0;
}
