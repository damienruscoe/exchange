#!/bin/bash

# This script runs the mbo-visualizer Docker container for each DBN file
# found in the test_data directory and its subdirectories.
# It generates unique SVG and PNG visualization files for each DBN file.

# Ensure the mbo-visualizer Docker image is built:
# docker build -t mbo-visualizer .

# Ensure the test_data directory exists and contains your DBN files.
# If you want to use the dummy mbo.dbn, make sure it's there.

# Define the base directory where DBN files are located
DBN_BASE_DIR="$(pwd)/resources/test_data"

# Define the output directory for visualizations
OUTPUT_DIR="$(pwd)/artifacts/mbo_vis"
mkdir -p "$OUTPUT_DIR"

echo "Starting visualization process for DBN files in $DBN_BASE_DIR..."
echo "Output visualizations will be saved to $OUTPUT_DIR"
echo "------------------------------------------------------------------"

# Find all .dbn files recursively in the DBN_BASE_DIR
find "$DBN_BASE_DIR" -name "*.dbn" | while read dbn_file; do
    # Get the relative path from DBN_BASE_DIR
    relative_path="${dbn_file#$DBN_BASE_DIR/}"
    
    # Generate a unique output prefix based on the relative path
    # Replace '/' with '_' and remove '.dbn' extension
    output_prefix=$(echo "$relative_path" | sed 's/\//_/g' | sed 's/\.dbn$//')
    
    echo "Processing: $dbn_file"
    echo "Output prefix: $output_prefix"
    
    start_time=$(date +%s)
    echo "Starting Docker run for $output_prefix at $(date)"

    timeout 300s docker run --rm \
      -v "$DBN_BASE_DIR:/app/test_data" \
      -v "$OUTPUT_DIR:/app/output" \
      mbo-visualizer \
      "/app/test_data/$relative_path" \
      --output_prefix "/app/output/$output_prefix"
    
    exit_code=$?
    end_time=$(date +%s)
    duration=$((end_time - start_time))

    if [ $exit_code -eq 124 ]; then
        echo "Docker run for $output_prefix timed out after 300 seconds."
    elif [ $exit_code -ne 0 ]; then
        echo "Docker run for $output_prefix failed with exit code $exit_code."
    else
        echo "Docker run for $output_prefix completed successfully in ${duration} seconds."
    fi
    echo "------------------------------------------------------------------"
done

echo "Visualization process complete. Check the '$OUTPUT_DIR' directory for your SVG and PNG files."
