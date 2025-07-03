import os
import sys

def process_spi_image(input_file, output_tag_file, output_primary_file):
    try:
        if not os.path.isfile(input_file):
            print(f"Error: Input file '{input_file}' not found.")
            return
        with open(input_file, "rb") as infile:
            data = infile.read()
        tag_data = data[:8]

        with open(output_tag_file, "wb") as tag_outfile:
            tag_outfile.write(tag_data)
        print(f"Created '{output_tag_file}' with the first 8 bytes.")
        # Extract data from offset 0x4000 to 0x81FFF for primary_image.bin
        primary_data = data[0x4000:0x82000]

        with open(output_primary_file, "wb") as primary_outfile:
            primary_outfile.write(primary_data)
        print(f"Created '{output_primary_file}' with data from offset 0x4000 to 0x81FFF.")
    except Exception as e:
        print(f"Error: {e}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python process_spi_image.py <input_file>")
        sys.exit(1)

    input_file = sys.argv[1]

    output_tag_file = "tag_base_addr.bin"       # First 8 bytes
    output_primary_file = "primary_image.bin"  # Data from 0x4000 to 0x81FFF

    # Process the input file
    process_spi_image(input_file, output_tag_file, output_primary_file)
