# GENERATED USING GEMINI

import os
import sys

try:
    from PIL import Image
except ImportError:
    print("Error: The 'Pillow' library is required.")
    print("Please install it by running: pip install Pillow")
    sys.exit(1)


def convert_to_bmp(input_path):
    """
    Converts a single image file to BMP format.
    The new .bmp file will be saved in the same directory.
    """
    if not os.path.exists(input_path):
        print(f"Error: File not found '{input_path}'")
        return

    # Create the output filename by replacing the extension with .bmp
    base_name = os.path.splitext(input_path)[0]
    output_path = base_name + ".bmp"

    try:
        # Open the image
        with Image.open(input_path) as img:
            # Save the image in BMP format
            # If the image has an alpha channel (like PNG),
            # converting to 'RGB' first ensures compatibility.
            if img.mode == "RGBA":
                img = img.convert("RGB")

            img.save(output_path, "BMP")

        print(f"Successfully converted: '{input_path}' -> '{output_path}'")

    except Exception as e:
        print(f"Error converting '{input_path}': {e}")


if __name__ == "__main__":
    # Get all arguments passed to the script (except the script name itself)
    files_to_convert = sys.argv[1:]

    if not files_to_convert:
        print("Usage: python convert_to_bmp.py [image1.png] [image2.jpg] ...")
        sys.exit(0)

    print(f"Starting batch conversion for {len(files_to_convert)} image(s)...")

    for image_file in files_to_convert:
        convert_to_bmp(image_file)

    print("Conversion complete.")
