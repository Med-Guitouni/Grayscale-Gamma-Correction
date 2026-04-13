def create_ppm(width, height, filename):
    # Generate a PPM file with specific RGB values [100, 75, 200]
    with open(filename, "wb") as f:
        f.write(f"P6\n{width} {height}\n255\n".encode('ascii'))
        f.write(bytearray([100, 75, 200] * (width * height)))  # RGB values


# Generate 50 PPM files
pixel_start = 4000000  # Start with 10,000 pixels
pixel_increment = 50000  # Increase by 1,000 pixels each time
num_images = 50  # Total number of PPM files

current_pixels = pixel_start

for i in range(1, num_images + 1):
    # Calculate width and height for the current image
    width = current_pixels // 100  # Example aspect ratio
    height = 100  # Fixed height

    # Generate the PPM file
    create_ppm(width, height, f"test_image_{i}.ppm")

    # Increase the pixel count for the next image
    current_pixels += pixel_increment

print(f"{num_images} PPM files generated with pixel counts increasing by {pixel_increment}.")