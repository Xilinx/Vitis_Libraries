#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace std;

// Function to calculate the maximum difference between corresponding values in two files
int max_difference(const string &file1, const string &file2) {
    ifstream f1(file1);
    ifstream f2(file2);

    // Check if both files opened successfully
    if (!f1.is_open()) {
        throw runtime_error("Error opening file: " + file1);
    }
    if (!f2.is_open()) {
        throw runtime_error("Error opening file: " + file2);
    }

    int max_diff = 0;
    int val1, val2;

    // Read values from both files line by line
    while (f1 >> val1 && f2 >> val2) {
        // Ensure the values are within the uint8_t range (0-255)
        if (val1 < 0 || val1 > 255 || val2 < 0 || val2 > 255) {
            throw invalid_argument("Invalid uint8_t value encountered: " + to_string(val1) + ", " + to_string(val2));
        }

        // Calculate the absolute difference
        int diff = abs(val1 - val2);

        // Update the maximum difference
        if (diff > max_diff) {
            max_diff = diff;
        }
    }

    // Check if the files had different number of lines
    if (f1 >> val1 || f2 >> val2) {
        throw runtime_error("The files have different number of values.");
    }

    return max_diff;
}

int main() {
    string file1 = "output_ref.txt";
    string file2 = "out.txt";

    try {
        int diff = max_difference(file1, file2);
        cout << "The maximum difference between the values in the two files is: " << diff << endl;
    } catch (const exception &e) {
        cerr << "An error occurred: " << e.what() << endl;
    }

    return 0;
}

