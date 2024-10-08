#ifndef SEGMENTATION_H
#define SEGMENTATION_H

#include <string>

class Segmentation {
public:
    static void segmentFiles(const std::string& inputFolder, const std::string& outputFolder);
};

#endif // SEGMENTATION_H
