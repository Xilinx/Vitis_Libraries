def max_difference(file1, file2):
    # Open both files
    with open(file1, 'r') as f1, open(file2, 'r') as f2:
        # Initialize the maximum difference
        max_diff = 0
        
        # Read and compare the values line by line
        for line1, line2 in zip(f1, f2):
            # Convert the lines to integers (uint8_t values)
            try:
                val1 = int(line1.strip())
                val2 = int(line2.strip())
                
                # Ensure the values are within uint8_t range (0-255)
                if not (0 <= val1 <= 255) or not (0 <= val2 <= 255):
                    raise ValueError(f"Invalid uint8_t value: {val1}, {val2}")
                
                # Calculate the absolute difference
                diff = abs(val1 - val2)
                
                # Update the maximum difference if necessary
                if diff > max_diff:
                    max_diff = diff
            except ValueError as e:
                print(f"Error processing lines: {line1.strip()}, {line2.strip()} - {e}")
                continue
        
        return max_diff

# Example usage
file1 = 'output_ref.txt'
file2 = 'out.txt'
try:
    diff = max_difference(file1, file2)
    print(f"The maximum difference between the values in the two files is: {diff}")
except Exception as e:
    print(f"An error occurred: {e}")

