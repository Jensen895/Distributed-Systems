import sys


def process_file(input_filename, file_pointer, num_lines, prefix, output_filename):
    # Subtract prefix to get the key
    key = input_filename[len(prefix) :]

    # Read the existing content of output file into a dictionary
    existing_data = {}
    try:
        with open(output_filename, "r") as output_file:
            for line in output_file:
                parts = line.strip().split(",", 1)
                existing_key = parts[0]
                existing_values = parts[1].split("|") if len(parts) > 1 else []
                existing_data[existing_key] = existing_values
    except FileNotFoundError:
        pass

    # Get values from input file
    values = []
    with open(input_filename, "r") as input_file:
        # Move to the specified file_pointer
        for _ in range(file_pointer):
            next(input_file)

        # Read num_lines lines
        for _ in range(num_lines):
            line = input_file.readline().strip()
            if line:
                values.append(line)

    # Update existing data or create new entry
    if key in existing_data:
        existing_data[key].extend(values)
    else:
        existing_data[key] = values

    # Write the updated dictionary to output file
    with open(output_filename, "w") as output_file:
        for k, v in existing_data.items():
            output_file.write(f"{k},{('|').join(v)}\n")


if __name__ == "__main__":
    if len(sys.argv) != 6:
        print(
            "Usage: python juice_exe.py input_filename file_pointer num_lines prefix output_filename"
        )
    else:
        input_filename = sys.argv[1]
        file_pointer = int(sys.argv[2])
        num_lines = int(sys.argv[3])
        prefix = sys.argv[4]
        output_filename = sys.argv[5]

        process_file(input_filename, file_pointer, num_lines, prefix, output_filename)
