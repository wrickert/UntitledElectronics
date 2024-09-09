import re

def process_header_file(input_file, output_file):
    # Read the input header file
    with open(input_file, 'r') as file:
        lines = file.readlines()

    # Prepare the output header content
    output_lines = []

    # Process each line to convert the pixel values
    for line in lines:
        # Find all hex values in the line
        hex_values = re.findall(r'0x[0-9A-Fa-f]{2}', line)

        # Replace each hex value according to the condition
        for hex_value in hex_values:
            # Convert hex value to integer
            int_value = int(hex_value, 16)
            # Replace with 0x00 if below 0x80, otherwise 0xFF
            if int_value < 0x80:
                new_value = '0'
            else:
                new_value = '1'
            # Replace in line
            line = line.replace(hex_value, new_value, 1)

        # Add the processed line to the output lines
        output_lines.append(line)

    # Write the output header file
    with open(output_file, 'w') as file:
        file.writelines(output_lines)

# Example usage
input_file = 'img-1.h'  # Replace with your input file name
output_file = 'output.h'  # Replace with your desired output file name
process_header_file(input_file, output_file)

