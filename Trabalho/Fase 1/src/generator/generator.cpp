#include <iostream>
#include <string>
#include <vector>
#include "primitives.h"

int main(int argc, char **argv) {
    if (argc > 1) {
        std::string prim = argv[1];
        std::vector<Vertex> verts;
        // O nome do ficheiro do output será o nome dado pelo utilizador,
        // o ficheiro vai ser guardado em models/generated tho
        if (prim == "plane" && argc == 5) {
            float dimension = std::stof(argv[2]);
            int divisions = std::stoi(argv[3]);
            std::string filename = argv[4];
            verts = generatePlane(dimension, divisions);
            writeVertices(verts, filename);
        } else if (prim == "sphere" && argc == 6) {
            float radius = std::stof(argv[2]);
            int slices = std::stoi(argv[3]);
            int stacks = std::stoi(argv[4]);
            std::string filename = argv[5];
            verts = generateSphere(radius, slices, stacks);
            writeVertices(verts, filename);
        } else if (prim == "box" && argc == 5) {
            float dimension = std::stof(argv[2]);
            int divisions = std::stoi(argv[3]);
            std::string filename = argv[4];
            verts = generateCube(dimension, divisions);
            writeVertices(verts, filename);
        } else if (prim == "cone" && argc == 7) {
            float bottomRadius = std::stof(argv[2]);
            float height = std::stof(argv[3]);
            int slices = std::stoi(argv[4]);
            int stacks = std::stoi(argv[5]);
            std::string filename = argv[6];
            verts = generateCone(bottomRadius, height, slices, stacks);
            writeVertices(verts, filename);
        } else {
            std::cerr << "Usage:\n"
                      << "  plane: generator plane dimension divisions outputfile\n"
                      << "  sphere: generator sphere radius slices stacks outputfile\n"
                      << "  box: generator box dimension divisions outputfile\n"
                      << "  cone: generator cone bottomRadius height slices stacks outputfile\n";
            return 1;
        }
        std::cout << "Primitive generated and saved in models/generated/ successfully." << std::endl;
        return 0;
    }

    // If no command-line parameters are provided, engine mode would be invoked.
    std::cerr << "No command-line parameters provided. Engine mode not implemented in this demo." << std::endl;
    return 1;
}
