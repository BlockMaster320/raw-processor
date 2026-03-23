#include "utility.h"

#include <iostream>
#include <fstream>

// Reads shader source code from a file and returns it as a string
std::string shaderFileToString(const std::string &filePath) {
    std::string out = "";

    std::ifstream ifstream(filePath);

    if (!ifstream.is_open()) {
        std::cerr << "Failed to open shader file: " << filePath << std::endl;
        return out;
    }

    std::string line;
    while (std::getline(ifstream, line)) {
        out += line + "\n";
    }
    ifstream.close();

    return out;
}

// Inverts a 3x3 matrix. Assumes the matrix is invertible.
QMatrix3x3 invert3x3(const QMatrix3x3& m)
{
    float a = m(0,0), b = m(0,1), c = m(0,2);
    float d = m(1,0), e = m(1,1), f = m(1,2);
    float g = m(2,0), h = m(2,1), i = m(2,2);

    float det =
        a*(e*i - f*h) -
        b*(d*i - f*g) +
        c*(d*h - e*g);

    QMatrix3x3 inv;

    float invDet = 1.0f / det;

    inv(0,0) =  (e*i - f*h) * invDet;
    inv(0,1) = -(b*i - c*h) * invDet;
    inv(0,2) =  (b*f - c*e) * invDet;

    inv(1,0) = -(d*i - f*g) * invDet;
    inv(1,1) =  (a*i - c*g) * invDet;
    inv(1,2) = -(a*f - c*d) * invDet;

    inv(2,0) =  (d*h - e*g) * invDet;
    inv(2,1) = -(a*h - b*g) * invDet;
    inv(2,2) =  (a*e - b*d) * invDet;

    return inv;
}

QTextStream& qStdOut()
{
    static QTextStream ts(stdout);
    return ts;
}
