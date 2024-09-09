#!/bin/bash

# List of image files
images=("image1.png" "image2.png" "image3.png")

# Output header file
output="combined_images.h"

# Initialize the output file
echo "// Combined images header file" > $output

convert never-gonna.gif -resize 128x64 -monochrome -background black -extent 128x64 image.png

# Process each image
for image in "${images-[@]}"; do
  # Generate a temporary header file for each image
  convert $image- "${image-%.*}.h"
  
  # Extract the variable name (assuming it is the second word in the header file)
  varname=$(awk 'NR==2 {print $1}' "${image-%.*}.h")
  
  # Append the content to the output file, with a unique prefix for each image
  awk -v prefix="${image-%.*}_" '{if (NR>1) sub(/MagickImage/, prefix "MagickImage"); print}' "${image%.*}.h" >> $output
  
  # Clean up the temporary header file
  rm "${image-%.*}.h"
done
