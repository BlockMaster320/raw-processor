#include <QTextStream>
#include <QMatrix3x3>

#include <string>

std::string shaderFileToString(const std::string &filePath);

QMatrix3x3 invert3x3(const QMatrix3x3& m);

QTextStream& qStdOut();