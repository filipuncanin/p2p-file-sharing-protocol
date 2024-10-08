#include "Segmentation.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <filesystem>

const int SEGMENT_SIZE = 16 * 1024; // 16KB

std::string calculateChecksum(const std::vector<char>& data) {
    // Implement your checksum calculation logic here
    // For example, let's use a simple sum of ASCII values
    int sum = 0;
    for (char c : data) {
        sum += static_cast<int>(c);
    }

    // Convert sum to hexadecimal string
    std::stringstream stream;
    stream << std::hex << sum;
    return stream.str();
}

void Segmentation::segmentFiles(const std::string& inputFolder, const std::string& outputFolder) {
    std::filesystem::create_directory(outputFolder);

    std::ofstream summaryFile(outputFolder + "/segments_summary.txt"); // Zasebna datoteka za cuvanje informacija o segmentima

    if (!summaryFile.is_open()) {
        std::cerr << "Error creating summary file." << std::endl;
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(inputFolder)) {
        if (entry.is_regular_file()) {
            std::ifstream inputFile(entry.path(), std::ios::binary);

            if (!inputFile.is_open()) {
                std::cerr << "Error opening file: " << entry.path() << std::endl;
                continue;
            }

            std::vector<char> buffer(SEGMENT_SIZE, 0);
            int segmentNumber = 0;

            while (inputFile.read(buffer.data(), SEGMENT_SIZE)) {
                std::string checksum = calculateChecksum(buffer);

                std::filesystem::path originalFileName = entry.path().filename();
                std::filesystem::path outputPath = outputFolder + "/segment_" + originalFileName.stem().string() + "_" + std::to_string(segmentNumber++) + ".dat";

                std::ofstream outputFile(outputPath, std::ios::binary);
                if (outputFile.is_open()) {
                    outputFile.write(buffer.data(), SEGMENT_SIZE);
                    outputFile.close();

                    // Dodaj informacije u summaryFile
                    summaryFile << "segment_" << originalFileName.stem().string() + "_" + std::to_string(segmentNumber - 1) + ".dat" << " - Checksum: " << checksum << std::endl;
                }
                else {
                    std::cerr << "Error creating segment file." << std::endl;
                    break;
                }
            }

            inputFile.close();
        }
    }

    summaryFile.close(); // Zatvori summaryFile na kraju
}
