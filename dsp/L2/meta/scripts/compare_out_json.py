#
# Copyright (C) 2024-2025, Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
import json
import sys


def load_json(file_path):
    """Load a JSON file and return its content."""
    with open(file_path, "r") as file:
        return json.load(file)


def compare_json(json1, json2, path=""):
    """Compare two JSON objects and print differences."""
    differences = []

    if isinstance(json1, dict) and isinstance(json2, dict):
        # Get all unique keys from both dictionaries
        all_keys = set(json1.keys()).union(set(json2.keys()))
        for key in all_keys:
            new_path = f"{path}.{key}" if path else key
            if key not in json1:
                differences.append(f"Missing in first JSON: {new_path}")
            elif key not in json2:
                differences.append(f"Missing in second JSON: {new_path}")
            else:
                differences.extend(compare_json(json1[key], json2[key], new_path))
    elif isinstance(json1, list) and isinstance(json2, list):
        # Compare lists (if applicable)
        if len(json1) != len(json2):
            differences.append(
                f"Different lengths at {path}: {len(json1)} != {len(json2)}"
            )
        else:
            # Sort lists for comparison
            sorted_json1 = sorted(
                json1,
                key=lambda x: (
                    json.dumps(x, sort_keys=True) if isinstance(x, dict) else x
                ),
            )
            sorted_json2 = sorted(
                json2,
                key=lambda x: (
                    json.dumps(x, sort_keys=True) if isinstance(x, dict) else x
                ),
            )
            for index, (item1, item2) in enumerate(zip(sorted_json1, sorted_json2)):
                differences.extend(compare_json(item1, item2, f"{path}[{index}]"))
    else:
        # Compare values
        if json1 != json2:
            differences.append(f"Different values at {path}: {json1} != {json2}")

    return differences


def main(file1, file2):
    """Main function to compare two JSON files."""
    try:
        json1 = load_json(file1)
        json2 = load_json(file2)
    except Exception as e:
        print(f"Error loading JSON files: {e}")
        sys.exit(1)

    print(f"Comparing {file1} and {file2}...\n")
    differences = compare_json(json1, json2)

    if differences:
        for difference in differences:
            print(difference)
        print("\nThe JSON files are different.")
        sys.exit(1)  # Exit with Error
    else:
        print("The JSON files are equivalent.")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python compare_json.py <file1.json> <file2.json>")
        sys.exit(1)

    file1 = sys.argv[1]
    file2 = sys.argv[2]
    main(file1, file2)
