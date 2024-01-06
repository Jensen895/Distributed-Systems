import sys


def process_file(input_filename, file_pointer):
    key_value_pairs = {}

    with open(input_filename, "r") as file:
        # Move the file pointer to the specified position
        file.seek(file_pointer)

        for line in file:
            # splitting each line by a delimiter
            parts = line.strip().split(",")
            if len(parts) >= 2:
                key, value = parts[0], parts[1]
                if key in key_value_pairs:
                    key_value_pairs[key].append(value)
                else:
                    key_value_pairs[key] = [value]

    return key_value_pairs


def main():
    if len(sys.argv) != 4:
        print(
            "Usage: python3 maple_exe.py <input_filename> <file_pointer> <output_filename>"
        )
        sys.exit(1)

    input_filename = sys.argv[1]
    file_pointer = int(sys.argv[2])  # Convert file_pointer to an integer
    output_filename = sys.argv[3]

    key_value_pairs = process_file(input_filename, file_pointer)

    # Output key-value pairs
    for key, values in key_value_pairs.items():
        output_filename += f"_{key.replace(' ', '_')}.txt"
        print(output_filename)
        with open(output_filename, "a") as output_file:
            for value in values:
                output_file.write(f"{value}\n")


if __name__ == "__main__":
    main()
